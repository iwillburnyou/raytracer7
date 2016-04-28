// -----------------------------------------------------------
// raytracer.cpp
// 2004 - Jacco Bikker - jacco@bik5.com - www.bik5.com -   <><
// -----------------------------------------------------------
// LATEST PERFORMANCE: 512x384, shark, 256 balls, 1.232 seconds
// (Intel compiler, full power)
// -----------------------------------------------------------

#include "raytracer.h"
#include "scene.h"
#include "common.h"
#include "windows.h"
#include "winbase.h"
#include "memory.h"

namespace Raytracer {

Ray::Ray( vector3& a_Origin, vector3& a_Dir ) : 
	m_Origin( a_Origin ), 
	m_Direction( a_Dir )
{
}

Engine::Engine()
{
	m_Scene = new Scene();
	KdTree::SetMemoryManager( new MManager() );
	m_Mod = new int[64];
	m_Mod = (int*)((((unsigned long)m_Mod) + 32) & (0xffffffff - 31));
	m_Mod[0] = 0, m_Mod[1] = 1, m_Mod[2] = 2, m_Mod[3] = 0, m_Mod[4] = 1;
	m_Stack = new kdstack[64];
	m_Stack = (kdstack*)((((unsigned long)m_Stack) + 32) & (0xffffffff - 31));
}

Engine::~Engine()
{
	delete m_Scene;
}

// -----------------------------------------------------------
// Engine::SetTarget
// Sets the render target canvas
// -----------------------------------------------------------
void Engine::SetTarget( Pixel* a_Dest, int a_Width, int a_Height )
{
	// set pixel buffer address & size
	m_Dest = a_Dest;
	m_Width = a_Width;
	m_Height = a_Height;
}

// -----------------------------------------------------------
// Engine::FindNearest
// Finds the nearest intersection in a KdTree for a ray
// -----------------------------------------------------------
int Engine::FindNearest( Ray& a_Ray, real& a_Dist, Primitive*& a_Prim )
{
	real tnear = 0, tfar = a_Dist, t;
	int retval = 0;
	vector3 p1 = m_Scene->GetExtends().GetPos();
	vector3 p2 = p1 + m_Scene->GetExtends().GetSize();
	vector3 D = a_Ray.GetDirection(), O = a_Ray.GetOrigin();
	for ( int i = 0; i < 3; i++ ) if (D.cell[i] < 0) 
	{
		if (O.cell[i] < p1.cell[i]) return 0;
	}
	else if (O.cell[i] > p2.cell[i]) return 0;
	// clip ray segment to box
	for (int i = 0; i < 3; i++)
	{
		real pos = O.cell[i] + tfar * D.cell[i];
		if (D.cell[i] < 0)
		{
			// clip end point
			if (pos < p1.cell[i]) tfar = tnear + (tfar - tnear) * ((O.cell[i] - p1.cell[i]) / (O.cell[i] - pos));
			// clip start point
			if (O.cell[i] > p2.cell[i]) tnear += (tfar - tnear) * ((O.cell[i] - p2.cell[i]) / (tfar * D.cell[i]));
		}
		else
		{
			// clip end point
			if (pos > p2.cell[i]) tfar = tnear + (tfar - tnear) * ((p2.cell[i] - O.cell[i]) / (pos - O.cell[i]));
			// clip start point
			if (O.cell[i] < p1.cell[i]) tnear += (tfar - tnear) * ((p1.cell[i] - O.cell[i]) / (tfar * D.cell[i]));
		}
		if (tnear > tfar) return 0;
	}
	// init stack
	int entrypoint = 0, exitpoint = 1;
	// init traversal
	KdTreeNode* farchild, *currnode;
	currnode = m_Scene->GetKdTree()->GetRoot();
	m_Stack[entrypoint].t = tnear;
	if (tnear > 0.0f) m_Stack[entrypoint].pb = O + D * tnear;
				 else m_Stack[entrypoint].pb = O;
	m_Stack[exitpoint].t = tfar;
	m_Stack[exitpoint].pb = O + D * tfar;
	m_Stack[exitpoint].node = 0;
	// traverse kd-tree
	while (currnode)
	{
		while (!currnode->IsLeaf())
		{
			real splitpos = currnode->GetSplitPos();
			int axis = currnode->GetAxis();
			if (m_Stack[entrypoint].pb.cell[axis] <= splitpos)
			{
				if (m_Stack[exitpoint].pb.cell[axis] <= splitpos)
				{
					currnode = currnode->GetLeft();
					continue;
				}
				if (m_Stack[exitpoint].pb.cell[axis] == splitpos)
				{
					currnode = currnode->GetRight();
					continue;
				}
				currnode = currnode->GetLeft();
				farchild = currnode + 1; // GetRight();
			}
			else
			{
				if (m_Stack[exitpoint].pb.cell[axis] > splitpos)
				{
					currnode = currnode->GetRight();
					continue;
				}
				farchild = currnode->GetLeft();
				currnode = farchild + 1; // GetRight();
			}
			t = (splitpos - O.cell[axis]) / D.cell[axis];
			int tmp = exitpoint++;
			if (exitpoint == entrypoint) exitpoint++;
			m_Stack[exitpoint].prev = tmp;
			m_Stack[exitpoint].t = t;
			m_Stack[exitpoint].node = farchild;
			m_Stack[exitpoint].pb.cell[axis] = splitpos;
			int nextaxis = m_Mod[axis + 1];
			int prevaxis = m_Mod[axis + 2];
			m_Stack[exitpoint].pb.cell[nextaxis] = O.cell[nextaxis] + t * D.cell[nextaxis];
			m_Stack[exitpoint].pb.cell[prevaxis] = O.cell[prevaxis] + t * D.cell[prevaxis];
		}
		ObjectList* list = currnode->GetList();
		real dist = m_Stack[exitpoint].t;
		while (list)
		{
			Primitive* pr = list->GetPrimitive();
			int result;
			m_Intersections++;
			if (result = pr->Intersect( a_Ray, dist ))
			{
				retval = result;
				a_Dist = dist;
				a_Prim = pr;
			}
			list = list->GetNext();
		}
		if (retval) return retval;
		entrypoint = exitpoint;
		currnode = m_Stack[exitpoint].node;
		exitpoint = m_Stack[entrypoint].prev;
	}
	return 0;
}

// -----------------------------------------------------------
// Engine::FindOccluder
// Finds any occluder between the origin and a light source
// -----------------------------------------------------------
int Engine::FindOccluder( Ray& a_Ray, real& a_Dist )
{
	real tnear = EPSILON, t;
	vector3 O, D = a_Ray.GetDirection();
	// init stack
	int entrypoint = 0, exitpoint = 1;
	// init traversal
	KdTreeNode* farchild, *currnode = m_Scene->GetKdTree()->GetRoot();
	m_Stack[entrypoint].t = tnear;
	m_Stack[entrypoint].pb = O = a_Ray.GetOrigin();
	m_Stack[exitpoint].t = a_Dist;
	m_Stack[exitpoint].pb = O + D * a_Dist;
	m_Stack[exitpoint].node = 0;
	// traverse kd-tree
	while (currnode)
	{
		while (!currnode->IsLeaf())
		{
			real splitpos = currnode->GetSplitPos();
			int axis = currnode->GetAxis();
			if (m_Stack[entrypoint].pb.cell[axis] <= splitpos)
			{
				if (m_Stack[exitpoint].pb.cell[axis] <= splitpos)
				{
					currnode = currnode->GetLeft();
					continue;
				}
				if (m_Stack[exitpoint].pb.cell[axis] == splitpos)
				{
					currnode = currnode->GetRight();
					continue;
				}
				currnode = currnode->GetLeft();
				farchild = currnode + 1; // GetRight();
			}
			else
			{
				if (m_Stack[exitpoint].pb.cell[axis] > splitpos)
				{
					currnode = currnode->GetRight();
					continue;
				}
				farchild = currnode->GetLeft();
				currnode = farchild + 1; // GetRight();
			}
			t = (splitpos - O.cell[axis]) / D.cell[axis];
			int tmp = exitpoint;
			if (++exitpoint == entrypoint) exitpoint++;
			m_Stack[exitpoint].prev = tmp;
			m_Stack[exitpoint].t = t;
			m_Stack[exitpoint].node = farchild;
			m_Stack[exitpoint].pb.cell[axis] = splitpos;
			int nextaxis = m_Mod[axis + 1];
			int prevaxis = m_Mod[axis + 2];
			m_Stack[exitpoint].pb.cell[nextaxis] = O.cell[nextaxis] + t * D.cell[nextaxis];
			m_Stack[exitpoint].pb.cell[prevaxis] = O.cell[prevaxis] + t * D.cell[prevaxis];
		}
		ObjectList* list = currnode->GetList();
		real dist = a_Dist; // m_Stack[exitpoint].t;
		while (list)
		{
			m_Intersections++;
			if (list->GetPrimitive()->Intersect( a_Ray, dist )) return 1;
			list = list->GetNext();
		}
		entrypoint = exitpoint;
		currnode = m_Stack[exitpoint].node;
		exitpoint = m_Stack[entrypoint].prev;
	}
	return 0;
}

// -----------------------------------------------------------
// Engine::Raytrace
// Naive ray tracing: Intersects the ray with every primitive
// in the scene to determine the closest intersection
// -----------------------------------------------------------
Primitive* Engine::Raytrace( Ray& a_Ray, Color& a_Acc, int a_Depth, real a_RIndex, real& a_Dist, real a_Samples, real a_SScale )
{
	// trace primary ray
	a_Dist = 10000.0f;
	Primitive* prim = 0;
	int result;
	// find the nearest intersection
	if (!(result = FindNearest( a_Ray, a_Dist, prim ))) return 0;
	// determine color at point of intersection
	vector3 pi = a_Ray.GetOrigin() + a_Ray.GetDirection() * a_Dist;
	Color color = prim->GetColor( pi );
	vector3 N = prim->GetNormal( pi );
	// trace lights
	for ( int l = 0; l < m_Scene->GetNrLights(); l++ )
	{
		Light* light = m_Scene->GetLight( l );
		// handle point light source
		vector3 L;
		real shade = CalcShade( light, pi, L, a_Samples, a_SScale );
		if (shade > 0)
		{
			// calculate diffuse shading
			if (prim->GetMaterial()->GetDiffuse() > 0)
			{
				real dot = DOT( L, N );
				if (dot > 0)
				{
					real diff = dot * prim->GetMaterial()->GetDiffuse() * shade;
					// add diffuse component to ray color
					a_Acc += diff * color * light->GetColor();
				}
			}
			// determine specular component using Schlick's BRDF approximation
			if (prim->GetMaterial()->GetSpecular() > 0)
			{
				// point light source: sample once for specular highlight
				vector3 R = L - 2.0f * DOT( L, N ) * N;
				real dot = DOT( a_Ray.GetDirection(), R );
				if (dot > 0)
				{
					real spec = dot * prim->GetMaterial()->GetSpecular() * shade / (50 - 50 * dot + dot);
					// add specular component to ray color
					a_Acc += spec * light->GetColor();
				}
			}
		}
	}
	// calculate reflection
	real refl = prim->GetMaterial()->GetReflection();
	if ((refl > 0.0f) && (a_Depth < TRACEDEPTH))
	{
		real drefl = prim->GetMaterial()->GetDiffuseRefl();
		if ((drefl > 0) && (a_Depth < 3))
		{
			// calculate diffuse reflection
			vector3 RP = a_Ray.GetDirection() - 2.0f * DOT( a_Ray.GetDirection(), N ) * N;
			vector3 RN1 = vector3( RP.z, RP.y, -RP.x );
			vector3 RN2 = RP.Cross( RN1 );
			refl *= a_SScale;
			for ( int i = 0; i < SAMPLES; i++ )
			{
				real xoffs, yoffs;
				do
				{
					xoffs = (m_Twister.Rand() - 0.5f) * drefl;
					yoffs = (m_Twister.Rand() - 0.5f) * drefl;
				}
				while ((xoffs * xoffs + yoffs * yoffs) > (drefl * drefl));
				vector3 R = RP + RN1 * xoffs + RN2 * yoffs * drefl;
				NORMALIZE( R );
				real dist;
				Color rcol( 0, 0, 0 );
				Raytrace( Ray( pi + R * EPSILON, R ), rcol, a_Depth + 1, a_RIndex, dist, a_Samples * 0.25f, a_SScale * 4 );
				m_RaysCast++;
				a_Acc += refl * rcol * color;
			}
		}
		else
		{
			// calculate perfect reflection
			vector3 N = prim->GetNormal( pi );
			vector3 R = a_Ray.GetDirection() - 2.0f * DOT( a_Ray.GetDirection(), N ) * N;
			Color rcol( 0, 0, 0 );
			real dist;
			Raytrace( Ray( pi + R * EPSILON, R ), rcol, a_Depth + 1, a_RIndex, dist, a_Samples * 0.5f, a_SScale * 2 );
			m_RaysCast++;
			a_Acc += refl * rcol * color;
		}
	}
	// calculate refraction
	real refr = prim->GetMaterial()->GetRefraction();
	if ((refr > 0) && (a_Depth < TRACEDEPTH))
	{
		real rindex = prim->GetMaterial()->GetRefrIndex();
		real n = a_RIndex / rindex;
		vector3 N = prim->GetNormal( pi ) * (real)result;
		real cosI = -DOT( N, a_Ray.GetDirection() );
		real cosT2 = 1.0f - n * n * (1.0f - cosI * cosI);
		if (cosT2 > 0.0f)
		{
			vector3 T = (n * a_Ray.GetDirection()) + (n * cosI - _sqrt( cosT2 )) * N;
			Color rcol( 0, 0, 0 );
			real dist;
			Raytrace( Ray( pi + T * EPSILON, T ), rcol, a_Depth + 1, rindex, dist, a_Samples * 0.5f, a_SScale * 2 );
			m_RaysCast++;
			// apply Beer's law
			Color absorbance = prim->GetMaterial()->GetColor() * 0.15f * -dist;
			Color transparency = Color( _exp( absorbance.r ), _exp( absorbance.g ), _exp( absorbance.b ) );
			a_Acc += rcol * transparency;
		}
	}
	// return pointer to primitive hit by primary ray
	return prim;
}

// -----------------------------------------------------------
// Engine::CalcShade
// Determines the light intensity received from a point light
// (in case of a SPHERE primitive) or an area light (in case
// of an AABB primitive)
// -----------------------------------------------------------
real Engine::CalcShade( Light* a_Light, vector3 a_IP, vector3& a_Dir, real a_Samples, real a_SScale )
{
	real retval;
	Primitive* prim = 0;
	if (a_Light->GetType() == Light::POINT)
	{
		// handle point light source
		retval = 0;
		a_Dir = a_Light->GetPos() - a_IP;
		real tdist = LENGTH( a_Dir );
		a_Dir *= (1.0f / tdist);
		tdist *= 1 - 4 * EPSILON;
		m_RaysCast++;
		if (!FindOccluder( Ray( a_IP + a_Dir * EPSILON, a_Dir ), tdist )) return 1;
	}
	else if (a_Light->GetType() == Light::AREA)
	{
		// Monte Carlo rendering
		retval = 0;
		a_Dir = a_Light->GetPos() - a_IP;
		NORMALIZE( a_Dir );
		vector3 deltax = a_Light->GetCellX(), deltay = a_Light->GetCellY();
		for ( int i = 0; i < a_Samples; i++ )
		{
			vector3 lp = a_Light->GetGrid( i & 15 ) + m_Twister.Rand() * deltax + m_Twister.Rand() * deltay;
			vector3 dir = lp - a_IP;
			real ldist = LENGTH( dir );
			dir *= 1.0f / ldist;
			ldist *= 1 - 4 * EPSILON;
			m_RaysCast++;
			if (!FindOccluder( Ray( a_IP + dir * EPSILON, dir ), ldist )) retval += a_SScale;
		}
	}
	return retval;
}

// -----------------------------------------------------------
// Engine::InitRender
// Initializes the renderer, by resetting the line / tile
// counters and precalculating some values
// -----------------------------------------------------------
void Engine::InitRender( vector3& a_Pos, vector3& a_Target )
{
	// set firts line to draw to
	m_CurrLine = 20;
	// set pixel buffer address of first pixel
	m_PPos = m_CurrLine * m_Width;
	// set eye and screen plane position
	m_Origin = vector3( 0, 0, -5 );
	m_P1 = vector3( -4,  3, 0 );
	m_P2 = vector3(  4,  3, 0 );
	m_P3 = vector3(  4, -3, 0 );
	m_P4 = vector3( -4, -3, 0 );
	// calculate camera matrix
	vector3 zaxis = a_Target - a_Pos;
	zaxis.Normalize();
	vector3 up( 0, 1, 0 );
	vector3 xaxis = up.Cross( zaxis );
	vector3 yaxis = xaxis.Cross( -zaxis );
	matrix m;
	m.cell[0] = xaxis.x, m.cell[1] = xaxis.y, m.cell[2] = xaxis.z;
	m.cell[4] = yaxis.x, m.cell[5] = yaxis.y, m.cell[6] = yaxis.z;
	m.cell[8] = zaxis.x, m.cell[9] = zaxis.y, m.cell[10] = zaxis.z;
	m.Invert();
	m.cell[3] = a_Pos.x, m.cell[7] = a_Pos.y, m.cell[11] = a_Pos.z;
	// move camera
	m_Origin = m.Transform( m_Origin );
	m_P1 = m.Transform( m_P1 );
	m_P2 = m.Transform( m_P2 );
	m_P3 = m.Transform( m_P3 );
	m_P4 = m.Transform( m_P4 );
	// calculate screen plane interpolation vectors
	m_DX = (m_P2 - m_P1) * (1.0f / m_Width);
	m_DY = (m_P4 - m_P1) * (1.0f / m_Height);
	// setup the tile renderer
	m_CurrCol = 0;
	m_CurrRow = 20 / TILESIZE;
	m_XTiles = m_Width / TILESIZE;
	m_YTiles = (m_Height - 40) / TILESIZE;
	// reset counters
	m_Intersections = 0;
	m_RaysCast = 0;
}

// -----------------------------------------------------------
// Engine::RenderRay
// Helper function, fires one ray in the regular grid
// -----------------------------------------------------------
Primitive* Engine::RenderRay( vector3 a_ScreenPos, Color& a_Acc )
{
	aabb e = m_Scene->GetExtends();
	vector3 dir = a_ScreenPos - m_Origin;
	NORMALIZE( dir );
	Color acc( 0, 0, 0 );
	Ray r( m_Origin, dir );
	m_RaysCast++;
	real dist;
	// trace ray
	return Raytrace( r, a_Acc, 1, 1.0f, dist, SAMPLES, 1.0f / SAMPLES );
}

// -----------------------------------------------------------
// Engine::Render
// Fires rays in the scene in a tile based fashion
// -----------------------------------------------------------
bool Engine::RenderTiles()
{
	// render scene in a tile based fashion
	aabb e = m_Scene->GetExtends();
	// initialize timer
	int msecs = GetTickCount();
	// render remaining tiles
	int tx = m_CurrCol, ty = m_CurrRow;
	int tdest = tx * TILESIZE + (ty * TILESIZE) * m_Width;
	vector3 tdir = m_P1 + (real)(tx * TILESIZE) * m_DX + (real)(ty * TILESIZE) * m_DY;
	while (1)
	{
		int dest = tdest;
		vector3 ldir = tdir;
		for ( int y = 0; y < TILESIZE; y++ )
		{
			vector3 pdir = ldir;
			for ( int x = 0; x < TILESIZE; x++ )
			{
				Color acc( 0, 0, 0 );
				Primitive* prim = RenderRay( pdir, acc );
				int red, green, blue;
				red = (int)(acc.r * 256);
				green = (int)(acc.g * 256);
				blue = (int)(acc.b * 256);
				if (red > 255) red = 255;
				if (green > 255) green = 255;
				if (blue > 255) blue = 255;
				m_Dest[dest++] = (red << 16) + (green << 8) + blue;
				pdir += m_DX;
			}
			ldir += m_DY;
			dest += (m_Width - TILESIZE);
		}
		tdest += TILESIZE;
		tdir += m_DX * TILESIZE;
		if (++tx == m_XTiles)
		{
			tx = 0;
			ty++;
			tdest = ty * TILESIZE * m_Width;
			tdir = m_P1 + (real)(ty * TILESIZE) * m_DY;
		}
		if (ty < m_YTiles)
		{
			if ((GetTickCount() - msecs) > 200) 
			{
				m_CurrCol = tx;
				m_CurrRow = ty;
				return false;
			}
		}
		else break;
	}
	return true;
}

}; // namespace Raytracer