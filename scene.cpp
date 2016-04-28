// -----------------------------------------------------------
// scene.cpp
// 2004 - Jacco Bikker - jacco@bik5.com - www.bik5.com -   <><
// -----------------------------------------------------------

#include "common.h"
#include "string.h"
#include "scene.h"
#include "raytracer.h"
#include "stdio.h"
#include "memory.h"
#include "surface.h"

namespace Raytracer {

MManager* KdTree::s_MManager = 0;

// -----------------------------------------------------------
// Texture class implementation
// -----------------------------------------------------------

Texture::Texture( Color* a_Bitmap, int a_Width, int a_Height ) :
	m_Bitmap( a_Bitmap ),
	m_Width( a_Width ), m_Height( a_Height )
{
}

Texture::Texture( char* a_File )
{
	FILE* f = fopen( a_File, "rb" );
	if (f)
	{
		// extract width and height from file header
		unsigned char buffer[20];
		fread( buffer, 1, 20, f );
		m_Width = *(buffer + 12) + 256 * *(buffer + 13);
		m_Height = *(buffer + 14) + 256 * *(buffer + 15);
		fclose( f );
		// read pixel data
		f = fopen( a_File, "rb" );
		unsigned char* t = new unsigned char[m_Width * m_Height * 3 + 1024];
		fread( t, 1, m_Width * m_Height * 3 + 1024, f );
		fclose( f );
		// convert RGB 8:8:8 pixel data to realing point RGB
		m_Bitmap = new Color[m_Width * m_Height];
		real rec = 1.0f / 256;
		for ( int size = m_Width * m_Height, i = 0; i < size; i++ )
			m_Bitmap[i] = Color( t[i * 3 + 20] * rec, t[i * 3 + 19] * rec, t[i * 3 + 18] * rec );
		delete t;
	}
}

Color Texture::GetTexel( real a_U, real a_V )
{
	// fetch a bilinearly filtered texel
	real fu = (a_U + 1000.5f) * m_Width;
	real fv = (a_V + 1000.0f) * m_Width;
	int u1 = ((int)fu) % m_Width;
	int v1 = ((int)fv) % m_Height;
	int u2 = (u1 + 1) % m_Width;
	int v2 = (v1 + 1) % m_Height;
	// calculate fractional parts of u and v
	real fracu = fu - _floor( fu );
	real fracv = fv - _floor( fv );
	// calculate weight factors
	real w1 = (1 - fracu) * (1 - fracv);
	real w2 = fracu * (1 - fracv);
	real w3 = (1 - fracu) * fracv;
	real w4 = fracu *  fracv;
	// fetch four texels
	Color c1 = m_Bitmap[u1 + v1 * m_Width];
	Color c2 = m_Bitmap[u2 + v1 * m_Width];
	Color c3 = m_Bitmap[u1 + v2 * m_Width];
	Color c4 = m_Bitmap[u2 + v2 * m_Width];
	// scale and sum the four colors
	return c1 * w1 + c2 * w2 + c3 * w3 + c4 * w4;
}

// -----------------------------------------------------------
// Material class implementation
// -----------------------------------------------------------

Material::Material() :
	m_Color( Color( 0.2f, 0.2f, 0.2f ) ),
	m_Refl( 0 ), m_Diff( 0.2f ), m_Spec( 0.8f ), 
	m_RIndex( 1.5f ), m_DRefl( 0 ), m_Texture( 0 ),
	m_UScale( 1.0f ), m_VScale( 1.0f )
{
}

void Material::SetUVScale( real a_UScale, real a_VScale )
{ 
	m_UScale = a_UScale; 
	m_VScale = a_VScale; 
	m_RUScale = 1.0f / a_UScale;
	m_RVScale = 1.0f / a_VScale;
}

void Material::SetParameters( real a_Refl, real a_Refr, Color& a_Col, real a_Diff, real a_Spec )
{
	m_Refl = a_Refl;
	m_Refr = a_Refr;
	m_Color = a_Col;
	m_Diff = a_Diff;
	m_Spec = a_Spec;
}

// -----------------------------------------------------------
// Primitive methods
// -----------------------------------------------------------

Primitive::Primitive( int a_Type, vector3& a_Centre, real a_Radius )
{
	m_Centre = a_Centre;
	m_SqRadius = a_Radius * a_Radius;
	m_Radius = a_Radius;
	m_RRadius = 1.0f / a_Radius;
	m_Type = a_Type;
	m_Material = new Material();
	// set vectors for texture mapping
	m_Vn = vector3( 0, 1, 0 );
	m_Ve = vector3( 1, 0, 0 );
	m_Vc = m_Vn.Cross( m_Ve );
}

Primitive::Primitive( int a_Type, Vertex* a_V1, Vertex* a_V2, Vertex* a_V3 )
{
	m_Type = a_Type;
	m_Material = 0;
	m_Vertex[0] = a_V1;
	m_Vertex[1] = a_V2;
	m_Vertex[2] = a_V3;
	// init precomp
	vector3 A = m_Vertex[0]->GetPos();
	vector3 B = m_Vertex[1]->GetPos();
	vector3 C = m_Vertex[2]->GetPos();
	vector3 c = B - A;
	vector3 b = C - A;
	m_N = b.Cross( c );
	int u, v;
	if (_fabs( m_N.x ) > _fabs( m_N.y))
	{
		if (_fabs( m_N.x ) > _fabs( m_N.z )) k = 0; else k = 2;
	}
	else
	{
		if (_fabs( m_N.y ) > _fabs( m_N.z )) k = 1; else k = 2;
	}
	u = (k + 1) % 3;
	v = (k + 2) % 3;
	// precomp
	real krec = 1.0f / m_N.cell[k];
	nu = m_N.cell[u] * krec;
	nv = m_N.cell[v] * krec;
	nd = m_N.Dot( A ) * krec;
	// first line equation
	real reci = 1.0f / (b.cell[u] * c.cell[v] - b.cell[v] * c.cell[u]);
	bnu = b.cell[u] * reci;
	bnv = -b.cell[v] * reci;
	// second line equation
	cnu = c.cell[v] * reci;
	cnv = -c.cell[u] * reci;
	// finalize normal
	m_N.Normalize();
	m_Vertex[0]->SetNormal( m_N );
	m_Vertex[1]->SetNormal( m_N );
	m_Vertex[2]->SetNormal( m_N );
}

Primitive::~Primitive()
{
	if (m_Type == SPHERE) delete m_Material;
}

unsigned int modulo[] = { 0, 1, 2, 0, 1 };
int Primitive::Intersect( Ray& a_Ray, real& a_Dist )
{
	if (m_Type == SPHERE)
	{
		vector3 v = a_Ray.GetOrigin() - m_Centre;
		real b = -DOT( v, a_Ray.GetDirection() );
		real det = (b * b) - DOT( v, v ) + m_SqRadius;
		int retval = MISS;
		if (det > 0)
		{
			det = _sqrt( det );
			real i1 = b - det;
			real i2 = b + det;
			if (i2 > 0)
			{
				if (i1 < 0) 
				{
					if (i2 < a_Dist) 
					{
						a_Dist = i2;
						retval = INPRIM;
					}
				}
				else
				{
					if (i1 < a_Dist)
					{
						a_Dist = i1;
						retval = HIT;
					}
				}
			}
		}
		return retval;
	}
	else
	{
		#define ku modulo[k + 1]
		#define kv modulo[k + 2]
		vector3 O = a_Ray.GetOrigin(), D = a_Ray.GetDirection(), A = m_Vertex[0]->GetPos();
		const real lnd = 1.0f / (D.cell[k] + nu * D.cell[ku] + nv * D.cell[kv]);
		const real t = (nd - O.cell[k] - nu * O.cell[ku] - nv * O.cell[kv]) * lnd;
		if (!(a_Dist > t && t > 0)) return MISS;
		real hu = O.cell[ku] + t * D.cell[ku] - A.cell[ku];
		real hv = O.cell[kv] + t * D.cell[kv] - A.cell[kv];
		real beta = m_U = hv * bnu + hu * bnv;
		if (beta < 0) return MISS;
		real gamma = m_V = hu * cnu + hv * cnv;
		if (gamma < 0) return MISS;
		if ((m_U + m_V) > 1) return MISS;
		a_Dist = t;
		return (DOT( D, m_N ) > 0)?INPRIM:HIT;
	}
}

vector3 Primitive::GetNormal( vector3& a_Pos ) 
{ 
	if (m_Type == SPHERE) 
	{
		return (a_Pos - m_Centre) * m_RRadius; 
	}
	else 
	{
		vector3 N1 = m_Vertex[0]->GetNormal();
		vector3 N2 = m_Vertex[1]->GetNormal();
		vector3 N3 = m_Vertex[2]->GetNormal();
		vector3 N = N1 + m_U * (N2 - N1) + m_V * (N3 - N1);
		NORMALIZE( N );
		return N;
	}
}
	
Color Primitive::GetColor( vector3& a_Pos )
{
	Color retval;
	if (!m_Material->GetTexture()) retval = m_Material->GetColor(); else
	{
		if (m_Type == SPHERE)
		{
			vector3 vp = (a_Pos - m_Centre) * m_RRadius;
			real phi = _acos( -DOT( vp, m_Vn ) );
			real u, v = phi * m_Material->GetVScaleReci() * (1.0f / PI);
			real theta = (_acos( DOT( m_Ve, vp ) / _sin( phi ))) * (2.0f / PI);
			if (DOT( m_Vc, vp ) >= 0) u = (1.0f - theta) * m_Material->GetUScaleReci();
								 else u = theta * m_Material->GetUScaleReci();
			retval = m_Material->GetTexture()->GetTexel( u, v ) * m_Material->GetColor();
		}
		else
		{
			real U1 = m_Vertex[0]->GetU(), V1 = m_Vertex[0]->GetV();		
			real U2 = m_Vertex[1]->GetU(), V2 = m_Vertex[1]->GetV();		
			real U3 = m_Vertex[2]->GetU(), V3 = m_Vertex[2]->GetV();		
			real u = U1 + m_U * (U2 - U1) + m_V * (U3 - U1);
			real v = V1 + m_U * (V2 - V1) + m_V * (V3 - V1);
			retval = m_Material->GetTexture()->GetTexel( u, v ) * m_Material->GetColor();
		}
	}
	return retval;
}

#define FINDMINMAX( x0, x1, x2, min, max ) \
  min = max = x0; if(x1<min) min=x1; if(x1>max) max=x1; if(x2<min) min=x2; if(x2>max) max=x2;
// X-tests
#define AXISTEST_X01( a, b, fa, fb )											\
	p0 = a * v0.cell[1] - b * v0.cell[2], p2 = a * v2.cell[1] - b * v2.cell[2]; \
    if (p0 < p2) { min = p0; max = p2;} else { min = p2; max = p0; }			\
	rad = fa * a_BoxHalfsize.cell[1] + fb * a_BoxHalfsize.cell[2];				\
	if (min > rad || max < -rad) return 0;
#define AXISTEST_X2( a, b, fa, fb )												\
	p0 = a * v0.cell[1] - b * v0.cell[2], p1 = a * v1.cell[1] - b * v1.cell[2];	\
    if (p0 < p1) { min = p0; max = p1; } else { min = p1; max = p0;}			\
	rad = fa * a_BoxHalfsize.cell[1] + fb * a_BoxHalfsize.cell[2];				\
	if(min>rad || max<-rad) return 0;
// Y-tests
#define AXISTEST_Y02( a, b, fa, fb )											\
	p0 = -a * v0.cell[0] + b * v0.cell[2], p2 = -a * v2.cell[0] + b * v2.cell[2]; \
    if(p0 < p2) { min = p0; max = p2; } else { min = p2; max = p0; }			\
	rad = fa * a_BoxHalfsize.cell[0] + fb * a_BoxHalfsize.cell[2];				\
	if (min > rad || max < -rad) return 0;
#define AXISTEST_Y1( a, b, fa, fb )												\
	p0 = -a * v0.cell[0] + b * v0.cell[2], p1 = -a * v1.cell[0] + b * v1.cell[2]; \
    if (p0 < p1) { min = p0; max = p1; } else { min = p1; max = p0; }			\
	rad = fa * a_BoxHalfsize.cell[0] + fb * a_BoxHalfsize.cell[2];				\
	if (min > rad || max < -rad) return 0;
// Z-tests
#define AXISTEST_Z12( a, b, fa, fb )											\
	p1 = a * v1.cell[0] - b * v1.cell[1], p2 = a * v2.cell[0] - b * v2.cell[1]; \
    if(p2 < p1) { min = p2; max = p1; } else { min = p1; max = p2; }			\
	rad = fa * a_BoxHalfsize.cell[0] + fb * a_BoxHalfsize.cell[1];				\
	if (min > rad || max < -rad) return 0;
#define AXISTEST_Z0( a, b, fa, fb )												\
	p0 = a * v0.cell[0] - b * v0.cell[1], p1 = a * v1.cell[0] - b * v1.cell[1];	\
    if(p0 < p1) { min = p0; max = p1; } else { min = p1; max = p0; }			\
	rad = fa * a_BoxHalfsize.cell[0] + fb * a_BoxHalfsize.cell[1];				\
	if (min > rad || max < -rad) return 0;

bool Primitive::PlaneBoxOverlap( vector3& a_Normal, vector3& a_Vert, vector3& a_MaxBox )
{
	vector3 vmin, vmax;
	for( int q = 0; q < 3; q++ )
	{
		float v = a_Vert.cell[q];
		if (a_Normal.cell[q] > 0.0f)
		{
			vmin.cell[q] = -a_MaxBox.cell[q] - v;
			vmax.cell[q] =  a_MaxBox.cell[q] - v;
		}
		else
		{
			vmin.cell[q] =  a_MaxBox.cell[q] - v;
			vmax.cell[q] = -a_MaxBox.cell[q] - v;
		}
	}
	if (DOT( a_Normal, vmin) > 0.0f) return false;
	if (DOT( a_Normal, vmax) >= 0.0f) return true;
	return false;
}

bool Primitive::IntersectTriBox( vector3& a_BoxCentre, vector3& a_BoxHalfsize, vector3& a_V0, vector3& a_V1, vector3& a_V2 )
{
	vector3 v0, v1, v2, normal, e0, e1, e2;
	float min, max, p0, p1, p2, rad, fex, fey, fez;
	v0 = a_V0 - a_BoxCentre;
	v1 = a_V1 - a_BoxCentre;
	v2 = a_V2 - a_BoxCentre;
	e0 = v1 - v0, e1 = v2 - v1, e2 = v0 - v2;
	fex = fabsf( e0.cell[0] );
	fey = fabsf( e0.cell[1] );
	fez = fabsf( e0.cell[2] );
	AXISTEST_X01( e0.cell[2], e0.cell[1], fez, fey );
	AXISTEST_Y02( e0.cell[2], e0.cell[0], fez, fex );
	AXISTEST_Z12( e0.cell[1], e0.cell[0], fey, fex );
	fex = fabsf( e1.cell[0] );
	fey = fabsf( e1.cell[1] );
	fez = fabsf( e1.cell[2] );
	AXISTEST_X01( e1.cell[2], e1.cell[1], fez, fey );
	AXISTEST_Y02( e1.cell[2], e1.cell[0], fez, fex );
	AXISTEST_Z0 ( e1.cell[1], e1.cell[0], fey, fex );
	fex = fabsf( e2.cell[0] );
	fey = fabsf( e2.cell[1] );
	fez = fabsf( e2.cell[2] );
	AXISTEST_X2 ( e2.cell[2], e2.cell[1], fez, fey );
	AXISTEST_Y1 ( e2.cell[2], e2.cell[0], fez, fex );
	AXISTEST_Z12( e2.cell[1], e2.cell[0], fey, fex );
	FINDMINMAX( v0.cell[0], v1.cell[0], v2.cell[0], min, max );
	if (min > a_BoxHalfsize.cell[0] || max < -a_BoxHalfsize.cell[0]) return false;
	FINDMINMAX( v0.cell[1], v1.cell[1], v2.cell[1], min, max );
	if (min > a_BoxHalfsize.cell[1] || max < -a_BoxHalfsize.cell[1]) return false;
	FINDMINMAX( v0.cell[2], v1.cell[2], v2.cell[2], min, max );
	if (min > a_BoxHalfsize.cell[2] || max < -a_BoxHalfsize.cell[2]) return false;
	normal = e0.Cross( e1 );
	if (!PlaneBoxOverlap( normal, v0, a_BoxHalfsize )) return false;
	return true;
}

bool Primitive::IntersectSphereBox( vector3& a_Centre, aabb& a_Box )
{
	float dmin = 0;
	vector3 spos = a_Centre;
	vector3 bpos = a_Box.GetPos();
	vector3 bsize = a_Box.GetSize();
	for ( int i = 0; i < 3; i++ )
	{
		if (spos.cell[i] < bpos.cell[i]) 
		{
			dmin = dmin + (spos.cell[i] - bpos.cell[i]) * (spos.cell[i] - bpos.cell[i]);
		}
		else if (spos.cell[i] > (bpos.cell[i] + bsize.cell[i])) 
		{
			dmin = dmin + (spos.cell[i] - (bpos.cell[i] + bsize.cell[i])) * (spos.cell[i] - (bpos.cell[i] + bsize.cell[i]));
		}
	}
	return (dmin <= m_SqRadius);
}

bool Primitive::IntersectBox( aabb& a_Box )
{
	if (m_Type == SPHERE)
	{
		return IntersectSphereBox( m_Centre, a_Box );
	}
	else
	{
		return IntersectTriBox( a_Box.GetPos() + a_Box.GetSize() * 0.5f, a_Box.GetSize() * 0.5f, 
								m_Vertex[0]->GetPos(), m_Vertex[1]->GetPos(), m_Vertex[2]->GetPos() );
	}
}

void Primitive::CalculateRange( real& a_Pos1, real& a_Pos2, int a_Axis )
{
	if (m_Type == SPHERE)
	{
		a_Pos1 = m_Centre.cell[a_Axis] - m_Radius;
		a_Pos2 = m_Centre.cell[a_Axis] + m_Radius;
	}
	else
	{
		vector3 pos1 = m_Vertex[0]->GetPos();
		a_Pos1 = pos1.cell[a_Axis], a_Pos2 = pos1.cell[a_Axis];
		for ( int i = 1; i < 3; i++ )
		{
			vector3 pos = m_Vertex[i]->GetPos();
			if (pos.cell[a_Axis] < a_Pos1) a_Pos1 = pos.cell[a_Axis];
			if (pos.cell[a_Axis] > a_Pos2) a_Pos2 = pos.cell[a_Axis];
		}
	}
}

// -----------------------------------------------------------
// Light class implementation
// -----------------------------------------------------------

Light::Light( int a_Type, vector3& a_P1, vector3& a_P2, vector3& a_P3, Color& a_Color )
{
	m_Type = a_Type;
	m_Color = a_Color;
	m_Grid = new vector3[16];
	m_Grid[ 0] = vector3( 1, 2, 0 );
	m_Grid[ 1] = vector3( 3, 3, 0 );
	m_Grid[ 2] = vector3( 2, 0, 0 );
	m_Grid[ 3] = vector3( 0, 1, 0 );
	m_Grid[ 4] = vector3( 2, 3, 0 );
	m_Grid[ 5] = vector3( 0, 3, 0 );
	m_Grid[ 6] = vector3( 0, 0, 0 );
	m_Grid[ 7] = vector3( 2, 2, 0 );
	m_Grid[ 8] = vector3( 3, 1, 0 );
	m_Grid[ 9] = vector3( 1, 3, 0 );
	m_Grid[10] = vector3( 1, 0, 0 );
	m_Grid[11] = vector3( 3, 2, 0 );
	m_Grid[12] = vector3( 2, 1, 0 );
	m_Grid[13] = vector3( 3, 0, 0 );
	m_Grid[14] = vector3( 1, 1, 0 );
	m_Grid[15] = vector3( 0, 2, 0 );
	m_CellX = (a_P2 - a_P1) * 0.25f;
	m_CellY = (a_P3 - a_P1) * 0.25f;
	for ( int i = 0; i < 16; i++ )
		m_Grid[i] = m_Grid[i].cell[0] * m_CellX + m_Grid[i].cell[1] * m_CellY + a_P1;
	m_Pos = a_P1 + 2 * m_CellX + 2 * m_CellY;
}

// -----------------------------------------------------------
// Scene class implementation
// -----------------------------------------------------------

Scene::Scene() :
	m_Primitives( 0 ), 
	m_Primitive( 0 ), 
	m_Extends( vector3( 0, 0, 0 ), vector3( 0, 0, 0 ) ), 
	m_State( 0 )
{
}

Scene::~Scene()
{
	delete m_Primitive;
}

void Scene::Load3DS( char* filename, Material* a_Material, vector3& a_Pos, real a_Size )
{
	// load 3ds file to memory
	FILE* f = fopen( filename, "rb" );
	char* memfile = new char[800 * 1024];
	unsigned long filelength = fread( memfile, 800 * 1024, 1, f );
	fclose( f );
	// initialize chunk parser
	m_Verts = m_TCoords = 0;
	m_Faces = 0;
	// process chunks
	EatChunk( memfile );
	delete memfile;
	// determine extends
	vector3 min( vector3( m_Verts[0], m_Verts[1], m_Verts[2] ) );
	vector3 max = min;
	for ( int i = 1; i < m_NrVerts; i++ )
	{
		if (m_Verts[i * 3] < min.x) min.x = m_Verts[i * 3];
		if (m_Verts[i * 3] > max.x) max.x = m_Verts[i * 3];
		if (m_Verts[i * 3 + 1] < min.y) min.y = m_Verts[i * 3 + 1];
		if (m_Verts[i * 3 + 1] > max.y) max.y = m_Verts[i * 3 + 1];
		if (m_Verts[i * 3 + 2] < min.z) min.z = m_Verts[i * 3 + 2];
		if (m_Verts[i * 3 + 2] > max.z) max.z = m_Verts[i * 3 + 2];
	}
	vector3 size = max - min;
	// determine scale based on largest extend
	real scale;
	if ((size.x > size.y) && (size.x > size.z)) scale = a_Size / size.x;
	else if (size.y > size.z) scale = a_Size / size.y; 
	else scale = a_Size / size.z;
	// determine offset
	vector3 centre = (min + max) * 0.5f;
	vector3 offset = (centre * scale) - a_Pos;
	// create vertices
	Vertex** vert = new Vertex*[m_NrVerts];
	Primitive*** vertface = new Primitive**[m_NrVerts];
	int* vertfaces = new int[m_NrVerts];
	for (int i = 0; i < m_NrVerts; i++ ) 
	{
		real x = m_Verts[i * 3], z = m_Verts[i * 3 + 1], y = m_Verts[i * 3 + 2];
		vert[i] = new Vertex( vector3( (x * scale) - offset.x, (y * scale ) - offset.y, (z * scale) - offset.z ), 0, 0 );
		vert[i]->SetUV( vert[i]->GetPos().cell[0] / a_Size, vert[i]->GetPos().cell[1] / a_Size );
		vertface[i] = new Primitive*[10];
		vertfaces[i] = 0;
	}
	// convert to ray tracer primitives
	for (int i = 0; i < m_NrFaces; i++)
	{
		int idx[3];
		for ( int v = 0; v < 3; v++ ) idx[v] = m_Faces[i * 3 + v];
		m_Primitive[m_Primitives] = new Primitive( Primitive::TRIANGLE, vert[idx[0]], vert[idx[1]], vert[idx[2]] );
		m_Primitive[m_Primitives++]->SetMaterial( a_Material );
		for (int v = 0; v < 1; v++) if (vertfaces[idx[v]] < 10) vertface[idx[v]][vertfaces[idx[v]]++] = m_Primitive[m_Primitives - 1];
	}
	// calculate vertex normals
	for (int i = 0; i < m_NrVerts; i++)
	{
		vector3 N( 0, 0, 0 );
		for ( int v = 0; v < vertfaces[i]; v++ ) N += vertface[i][v]->GetNormal();
		N *= 1.0f / vertfaces[i];
		vert[i]->SetNormal( N );
		delete vertface[i];
	}
	delete vertface;
	delete vertfaces;
	delete vert;
}

unsigned char* temp = new unsigned char[8];
unsigned char* aligned = (unsigned char*)(((unsigned long)temp + 4) & 0xFFFFFFFC);
inline float getf( char* addr )
{
	memcpy( aligned, addr, 4 );
	return *(float*)aligned;
}
inline unsigned short getw( char* addr )
{
	memcpy( aligned, addr, 2 );
	return *(unsigned short*)aligned;
}
inline unsigned long getd( char* addr )
{
	memcpy( aligned, addr, 4 );
	return *(unsigned long*)aligned;
}

long Scene::EatChunk( char* buffer )
{
	short chunkid = getw( buffer );
	long chunklength = getd( buffer + 2 );
	int j, i = 6, cp = 0, tcoords = 0;
	switch (chunkid)
	{
	case 0x4D4D:
		while ((getw( buffer + i) != 0x3D3D) && (getw( buffer + i) != 0xB000)) i += 2;
		break;
	case 0x4000:
		while (*(buffer + (i++)) != 0);
		break;
	case 0x4110:
		m_NrVerts = getw( buffer + i );
		delete m_Verts;
		m_Verts = new float[m_NrVerts * 3];
		i += 2;
		for ( j = 0; j < m_NrVerts; j++ )
		{
			m_Verts[j * 3] = getf( buffer + i );
			m_Verts[j * 3 + 1] = getf( buffer + i + 4 );
			m_Verts[j * 3 + 2] = getf( buffer + i + 8 );
			i += 12;
		}
		break;
	case 0x4120:
		m_NrFaces = getw( buffer + i );
		delete m_Faces;
		m_Faces = new unsigned short[m_NrFaces * 3];
		i += 2;
		for ( j = 0; j < m_NrFaces; j++ )
		{
			m_Faces[j * 3] = getw( buffer + i );
			m_Faces[j * 3 + 1] = getw( buffer + i + 2 );
			m_Faces[j * 3 + 2] = getw( buffer + i + 4 );
			i += 8;
		}
	case 0x4140:
		tcoords = getw( buffer + i );
		delete m_TCoords;
		m_TCoords = new float[tcoords * 2];
		i += 2;
		for ( j = 0; j < tcoords; j++ )
		{
			m_TCoords[j * 2] = getf( buffer + i );
			m_TCoords[j * 2 + 1] = getf( buffer + i + 4 );
			i += 8;
		}
	case 0x3D3D:
	case 0x4100:
		break;
	default:
		i = chunklength;
	break;
	}
	while (i < chunklength) i += EatChunk( buffer + i );
	return chunklength;
}

void Scene::AddBox( vector3 a_Pos, vector3 a_Size )
{
	Vertex* v[8];
	v[0] = new Vertex( vector3( a_Pos.x, a_Pos.y, a_Pos.z ), 0, 0 );
	v[1] = new Vertex( vector3( a_Pos.x + a_Size.x, a_Pos.y, a_Pos.z ), 0, 0 );
	v[2] = new Vertex( vector3( a_Pos.x + a_Size.x, a_Pos.y + a_Size.y, a_Pos.z ), 0, 0 );
	v[3] = new Vertex( vector3( a_Pos.x, a_Pos.y + a_Size.y, a_Pos.z ), 0, 0 );
	v[4] = new Vertex( vector3( a_Pos.x, a_Pos.y, a_Pos.z + a_Size.z ), 0, 0 );
	v[5] = new Vertex( vector3( a_Pos.x + a_Size.x, a_Pos.y, a_Pos.z + a_Size.z ), 0, 0 );
	v[6] = new Vertex( vector3( a_Pos.x + a_Size.x, a_Pos.y + a_Size.y, a_Pos.z + a_Size.z ), 0, 0 );
	v[7] = new Vertex( vector3( a_Pos.x, a_Pos.y + a_Size.y, a_Pos.z + a_Size.z ), 0, 0 );
	// front plane
	m_Primitive[m_Primitives++] = new Primitive( Primitive::TRIANGLE, v[0], v[1], v[3] );
	m_Primitive[m_Primitives++] = new Primitive( Primitive::TRIANGLE, v[1], v[2], v[3] );
	// back plane
	m_Primitive[m_Primitives++] = new Primitive( Primitive::TRIANGLE, v[5], v[4], v[7] );
	m_Primitive[m_Primitives++] = new Primitive( Primitive::TRIANGLE, v[5], v[7], v[6] );
	// left plane
	m_Primitive[m_Primitives++] = new Primitive( Primitive::TRIANGLE, v[4], v[0], v[3] );
	m_Primitive[m_Primitives++] = new Primitive( Primitive::TRIANGLE, v[4], v[3], v[7] );
	// right plane
	m_Primitive[m_Primitives++] = new Primitive( Primitive::TRIANGLE, v[1], v[5], v[2] );
	m_Primitive[m_Primitives++] = new Primitive( Primitive::TRIANGLE, v[5], v[6], v[2] );
	// top plane
	m_Primitive[m_Primitives++] = new Primitive( Primitive::TRIANGLE, v[4], v[5], v[1] );
	m_Primitive[m_Primitives++] = new Primitive( Primitive::TRIANGLE, v[4], v[1], v[0] );
	// bottom plane
	m_Primitive[m_Primitives++] = new Primitive( Primitive::TRIANGLE, v[6], v[7], v[2] );
	m_Primitive[m_Primitives++] = new Primitive( Primitive::TRIANGLE, v[7], v[3], v[2] );
}

void Scene::AddPlane( vector3 a_P1, vector3 a_P2, vector3 a_P3, vector3 a_P4, Material* a_Mat )
{
	Vertex* v[4];
	v[0] = new Vertex( a_P1, 0, 0 );
	v[1] = new Vertex( a_P2, 0, 0 );
	v[2] = new Vertex( a_P3, 0, 0 );
	v[3] = new Vertex( a_P4, 0, 0 );
	m_Primitive[m_Primitives] = new Primitive( Primitive::TRIANGLE, v[0], v[1], v[3] );
	m_Primitive[m_Primitives++]->SetMaterial( a_Mat );
	m_Primitive[m_Primitives] = new Primitive( Primitive::TRIANGLE, v[1], v[2], v[3] );
	m_Primitive[m_Primitives++]->SetMaterial( a_Mat );
}

bool Scene::InitScene( Surface* a_MsgSurf )
{
	Material* mat;
	int x;
	vector3 p1, p2;
	switch (m_State)
	{
	case 0:
		a_MsgSurf->Print( "constructing geometry", 2, 2, 0xffffffff );
		break;
	case 1:
		m_Primitive = new Primitive*[20000];
		m_Primitives = 0;
		m_Light = new Light*[MAXLIGHTS];
		m_Lights = 0;
		// ground plane
		mat = new Material();
		mat->SetParameters( 0.0f, 0.0f, Color( 0.4f, 0.3f, 0.3f ), 1.0f, 0.0f );
		AddPlane( vector3( -13, -4.4f, -5.5f ), vector3( 13, -4.4f, -5.5f ),
				  vector3(  13, -4.4f, 29 ), vector3( -13, -4.4f, 29 ), mat );
		// back plane
		mat = new Material();
		mat->SetParameters( 0.0f, 0.0f, Color( 0.5f, 0.3f, 0.5f ), 0.6f, 0.0f );
		AddPlane( vector3( -13, -4.4f, 8 ), vector3( 13, -4.4f, 16 ),
				  vector3(  13,  7.4f, 16 ), vector3( -13, 7.4f, 8 ), mat );
		// ceiling plane
		mat = new Material();
		mat->SetParameters( 0.0f, 0.0f, Color( 0.4f, 0.7f, 0.7f ), 0.5f, 0.0f );
		AddPlane( vector3(  13, 7.4f, -5.5f ), vector3( -13, 7.4f, -5.5f ),
				  vector3( -13, 7.4f, 29 ), vector3( 13, 7.4f, 29 ), mat );
		// big sphere
		// m_Primitive[m_Primitives] = new Primitive( Primitive::SPHERE, vector3( 2, 0.8f, 3 ), 2.5f );
		// m_Primitive[m_Primitives++]->GetMaterial()->SetParameters( 0.2f, 0.8f, Color( 0.7f, 0.7f, 1.0f ), 0.2f, 0.8f );
		// small sphere
		m_Primitive[m_Primitives] = new Primitive( Primitive::SPHERE, vector3( -5.5f, -0.5, 7 ), 2 );
		m_Primitive[m_Primitives++]->GetMaterial()->SetParameters( 0.5f, 0.0f, Color( 0.7f, 0.7f, 1.0f ), 0.1f, 0.8f );
	#if 0
		// area lights
		m_Light[m_Lights++] = new Light( Light::AREA, vector3( -1, 6, 4 ), vector3( 1, 6, 4 ), vector3( -1, 6, 6 ), Color( 0.7f, 0.7f, 0.7f ) );
		m_Light[m_Lights++] = new Light( Light::AREA, vector3( -1, 6, -1 ), vector3( 1, 6, -1 ), vector3( -1, 6, 1 ), Color( 0.7f, 0.7f, 0.7f ) );
	#else
		// point lights
		m_Light[m_Lights++] = new Light( Light::POINT, vector3( 0, 5, 5 ), Color( 0.4f, 0.4f, 0.4f ) );
		m_Light[m_Lights++] = new Light( Light::POINT, vector3( -3, 5, 1 ), Color( 0.6f, 0.6f, 0.8f ) );
	#endif
		// extra sphere
		m_Primitive[m_Primitives] = new Primitive( Primitive::SPHERE, vector3( -1.5f, -3.8f, 1 ), 1.5f );
		m_Primitive[m_Primitives++]->GetMaterial()->SetParameters( 0.0f, 0.8f, Color( 1.0f, 0.4f, 0.4f ), 0.2f, 0.8f );
		// grid
		for (int x = 0; x < 8; x++) for (int y = 0; y < 7; y++)
		{
			m_Primitive[m_Primitives] = new Primitive( Primitive::SPHERE, vector3( -4.5f + x * 1.5f, -4.3f + y * 1.5f, 10 ), 0.3f );
			m_Primitive[m_Primitives++]->GetMaterial()->SetParameters( 0.0f, 0.0f, Color( 0.3f, 1.0f, 0.4f ), 0.6f, 0.6f );
		}
		for (int x = 0; x < 8; x++) for (int y = 0; y < 8; y++)
		{
			m_Primitive[m_Primitives] = new Primitive( Primitive::SPHERE, vector3( -4.5f + x * 1.5f, -4.3f, 10.0f - y * 1.5f ), 0.3f );
			m_Primitive[m_Primitives++]->GetMaterial()->SetParameters( 0.0f, 0.0f, Color( 0.3f, 1.0f, 0.4f ), 0.6f, 0.6f );
		}
		for (int x = 0; x < 16; x++) for (int y = 0; y < 8; y++)
		{
			m_Primitive[m_Primitives] = new Primitive( Primitive::SPHERE, vector3( -8.5f + x * 1.5f, 4.3f, 10.0f - y ), 0.3f );
			m_Primitive[m_Primitives++]->GetMaterial()->SetParameters( 0.0f, 0.0f, Color( 0.3f, 1.0f, 0.4f ), 0.6f, 0.6f );
		}
		mat = new Material();
		mat->SetParameters( 0.9f, 0, Color( 0.9f, 0.9f, 1 ), 0.3f, 0.7f );
		mat->SetRefrIndex( 1.3f );
		// mat->SetTexture( new Texture( "textures/wood.tga" ) );
		Load3DS( "meshes/knot.3ds", mat, vector3( 0, 0.5f, 4 ), 6 );
		break;
	case 2:
		a_MsgSurf->Print( "building kdtree - please wait", 2, 10, 0xffffffff );
		break;
	case 3:
		// build the kd-tree
		p1 = vector3( -14, -6, -6 ), p2 = vector3( 14, 8, 30 );
		m_Extends = aabb( p1, p2 - p1 );
		m_KdTree = new KdTree();
		m_KdTree->Build( this );
		break;
	default:
		return true;
	};
	m_State++;
	return false;
}

// -----------------------------------------------------------
// KdTree class implementation
// -----------------------------------------------------------
KdTree::KdTree()
{
	m_Root = new KdTreeNode();
}

void KdTree::Build( Scene* a_Scene )
{
	for ( int p = 0; p < a_Scene->GetNrPrimitives(); p++ )
		m_Root->Add( a_Scene->GetPrimitive( p ) );
	int prims = a_Scene->GetNrPrimitives();
	aabb sbox = a_Scene->GetExtends();
	m_SPool = new SplitList[prims * 2 + 8];
	int i;
	for (  i = 0; i < (prims * 2 + 6); i++ ) m_SPool[i].next = &m_SPool[i + 1];
	m_SPool[i].next = 0;
	m_SList = 0;
	Subdivide( m_Root, sbox, 0, prims );
}

void KdTree::InsertSplitPos( real a_SplitPos )
{
	// insert a split position candidate in the list if unique
	SplitList* entry = m_SPool;
	m_SPool = m_SPool->next;
	entry->next = 0;
	entry->splitpos = a_SplitPos;
	entry->n1count = 0;
	entry->n2count = 0;
	if (!m_SList) m_SList = entry; else
	{
		if (a_SplitPos < m_SList->splitpos)
		{
			entry->next = m_SList;
			m_SList = entry;
		}
		else if (a_SplitPos == m_SList->splitpos)
		{
			entry->next = m_SPool; // redundant; recycle
			m_SPool = entry;
		}
		else
		{
			SplitList* list = m_SList;
			while ((list->next) && (a_SplitPos >= list->next->splitpos)) 
			{
				if (a_SplitPos == list->next->splitpos)
				{
					entry->next = m_SPool; // redundant; recycle
					m_SPool = entry;
					return;
				}
				list = list->next;
			}
			entry->next = list->next;
			list->next = entry;
		}
	}
}

void KdTree::Subdivide( KdTreeNode* a_Node, aabb& a_Box, int a_Depth, int a_Prims )
{
	// recycle used split list nodes
	if (m_SList)
	{
		SplitList* list = m_SList;
		while (list->next) list = list->next;
		list->next = m_SPool;
		m_SPool = m_SList, m_SList = 0;
	}
	// determine split axis
	vector3 s = a_Box.GetSize();
	if ((s.x >= s.y) && (s.x >= s.z)) a_Node->SetAxis( 0 );
	else if ((s.y >= s.x) && (s.y >= s.z)) a_Node->SetAxis( 1 );
	int axis = a_Node->GetAxis();
	// make a list of the split position candidates
	ObjectList* l = a_Node->GetList();
	real p1, p2;
	real pos1 = a_Box.GetPos().cell[axis];
	real pos2 = a_Box.GetPos().cell[axis] + a_Box.GetSize().cell[axis];
	bool* pright = new bool[a_Prims];
	float* eleft = new float[a_Prims], *eright = new float[a_Prims];
	Primitive** parray = new Primitive*[a_Prims];
	int aidx = 0;
	while (l)
	{
		Primitive* p = parray[aidx] = l->GetPrimitive();
		pright[aidx] = true;
		p->CalculateRange( eleft[aidx], eright[aidx], axis );
		aidx++;
		if (p->GetType() == Primitive::SPHERE)
		{
			p1 = p->GetCentre().cell[axis] - p->GetRadius();
			p2 = p->GetCentre().cell[axis] + p->GetRadius();
			if ((p1 >= pos1) && (p1 <= pos2)) InsertSplitPos( p1 );
			if ((p2 >= pos1) && (p2 <= pos2)) InsertSplitPos( p2 );
		}
		else
		{
			for ( int i = 0; i < 3; i++ )
			{
				p1 = p->GetVertex( i )->GetPos().cell[axis];
				if ((p1 >= pos1) && (p1 <= pos2)) InsertSplitPos( p1 );
			}
		}
		l = l->GetNext();
	}
	// determine n1count / n2count for each split position
	aabb b1, b2, b3 = a_Box, b4 = a_Box;
	SplitList* splist = m_SList;
	float b3p1 = b3.GetPos().cell[axis];
	float b4p2 = b4.GetPos().cell[axis] + b4.GetSize().cell[axis];
	while (splist)
	{
		b4.GetPos().cell[axis] = splist->splitpos;
		b4.GetSize().cell[axis] = pos2 - splist->splitpos;
		b3.GetSize().cell[axis] = splist->splitpos - pos1;
		float b3p2 = b3.GetPos().cell[axis] + b3.GetSize().cell[axis];
		float b4p1 = b4.GetPos().cell[axis];
		for ( int i = 0; i < a_Prims; i++ ) if (pright[i])
		{
			Primitive* p = parray[i];
			if ((eleft[i] <= b3p2) && (eright[i] >= b3p1))
				if (p->IntersectBox( b3 )) splist->n1count++;
			if ((eleft[i] <= b4p2) && (eright[i] >= b4p1))
				if (p->IntersectBox( b4 )) splist->n2count++; else pright[i] = false;
		}
		else splist->n1count++;
		splist = splist->next;
	}
	delete pright;
	// calculate surface area for current node
	real SAV = 0.5f / (a_Box.w() * a_Box.d() + a_Box.w() * a_Box.h() + a_Box.d() * a_Box.h());
	// calculate cost for not splitting
	real Cleaf = a_Prims * 1.0f;
	// determine optimal split plane position
	splist = m_SList;
	real lowcost = 10000;
	real bestpos = 0;
	while (splist)
	{
		// calculate child node extends
		b4.GetPos().cell[axis] = splist->splitpos;
		b4.GetSize().cell[axis] = pos2 - splist->splitpos;
		b3.GetSize().cell[axis] = splist->splitpos - pos1;
		// calculate child node cost
		real SA1 = 2 * (b3.w() * b3.d() + b3.w() * b3.h() + b3.d() * b3.h());
		real SA2 = 2 * (b4.w() * b4.d() + b4.w() * b4.h() + b4.d() * b4.h());
		real splitcost = 0.3f + 1.0f * (SA1 * SAV * splist->n1count + SA2 * SAV * splist->n2count);
		// update best cost tracking variables
		if (splitcost < lowcost)
		{
			lowcost = splitcost;
			bestpos = splist->splitpos;
			b1 = b3, b2 = b4;
		}
		splist = splist->next;
	}
	if (lowcost > Cleaf) return;
	a_Node->SetSplitPos( bestpos );
	// construct child nodes
	KdTreeNode* left = s_MManager->NewKdTreeNodePair();
	int n1count = 0, n2count = 0, total = 0;
	// assign primitives to both sides
	float b1p1 = b1.GetPos().cell[axis];
	float b2p2 = b2.GetPos().cell[axis] + b2.GetSize().cell[axis];
	float b1p2 = b1.GetPos().cell[axis] + b1.GetSize().cell[axis];
	float b2p1 = b2.GetPos().cell[axis];
	for ( int i = 0; i < a_Prims; i++ )
	{
		Primitive* p = parray[i];
		total++;
		if ((eleft[i] <= b1p2) && (eright[i] >= b1p1)) if (p->IntersectBox( b1 )) 
		{
			left->Add( p );
			n1count++;
		}
		if ((eleft[i] <= b2p2) && (eright[i] >= b2p1)) if (p->IntersectBox( b2 )) 
		{
			(left + 1)->Add( p );
			n2count++;
		}
	}
	delete eleft;
	delete eright;
	delete parray;
	s_MManager->FreeObjectList( a_Node->GetList() );
	a_Node->SetLeft( left );
	a_Node->SetLeaf( false );
	if (a_Depth < MAXTREEDEPTH)
	{
		if (n1count > 2) Subdivide( left, b1, a_Depth + 1, n1count );
		if (n2count > 2) Subdivide( left + 1, b2, a_Depth + 1, n2count );
	}
}

// -----------------------------------------------------------
// KdTreeNode class implementation
// -----------------------------------------------------------

void KdTreeNode::Add( Primitive* a_Prim )
{
	ObjectList* lnode = KdTree::s_MManager->NewObjectList();
	lnode->SetPrimitive( a_Prim );
	lnode->SetNext( GetList() );
	SetList( lnode );
}

}; // namespace Raytracer