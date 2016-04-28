// -----------------------------------------------------------
// memory.h
// 2004 - Jacco Bikker - jacco@bik5.com - www.bik5.com -   <><
// -----------------------------------------------------------

#include "memory.h"
#include "scene.h"

namespace Raytracer {

MManager::MManager() : m_OList( 0 )
{
	// build a 32-byte aligned array of KdTreeNodes
	m_KdArray = (char*)(new KdTreeNode[1000000]);
	m_ObjArray = (char*)(new ObjectList[100000]);
	unsigned long addr = (unsigned long)m_KdArray;
	m_KdPtr = (KdTreeNode*)((addr + 32) & (0xffffffff - 31));
	addr = (unsigned long)m_ObjArray;
	m_ObjPtr = (ObjectList*)((addr + 32) & (0xffffffff - 31));
	ObjectList* ptr = m_ObjPtr;
	for ( int i = 0; i < 99995; i++ ) 
	{
		ptr->SetNext( ptr + 1 );
		ptr++;
	}
	ptr->SetNext( 0 );
	m_OList = m_ObjPtr;
}

ObjectList* MManager::NewObjectList()
{
	ObjectList* retval;
	retval = m_OList;
	m_OList = m_OList->GetNext();
	retval->SetNext( 0 );
	retval->SetPrimitive( 0 );
	return retval;
}

void MManager::FreeObjectList( ObjectList* a_List )
{
	ObjectList* list = a_List;
	while (list->GetNext()) list = list->GetNext();
	list->SetNext( m_OList );
	m_OList = a_List;
}

KdTreeNode* MManager::NewKdTreeNodePair()
{ 
	unsigned long* tmp = (unsigned long*)m_KdPtr;
	tmp[1] = tmp[3] = 6;
	KdTreeNode* node = m_KdPtr;
	m_KdPtr += 2;
	return node;
}

}; // namespace Raytracer
