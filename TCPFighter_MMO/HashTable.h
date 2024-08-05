#pragma once


/*
 A descriptor structure for the bucket that the user inserts.

 link : Member variable used to traverse the linked list of BUCKET_DESCRIPTOR.(head - pBucketDescHead_, tail - pBucketDescTail_)
 this Member variable is used When traversing buckets using GetFirst and GetNext
 if all buckets with dwLinkNodeNum of the current BUCKET_DESCRIPTOR have been traversed
 it is used to find the next BUCKET_DESCRIPTOR with non-zero dwLinkNodeNum.

pBucketLinkHead, pBucketLinkTail : head, tail of the linked list of buckets managed by BUCKET_DESCRIPTOR 
(so each bucket has the same hash value for the key)
*/
struct BUCKET_DESCRIPTOR
{
	LINKED_NODE link;
	LINKED_NODE* pBucketLinkHead = nullptr;
	LINKED_NODE* pBucketLinkTail = nullptr;
	DWORD dwLinkNodeNum = 0;
};

/*
The structure must be declared in the structure that the user will insert into the hashtable in order to avoid dynamic memory allocation by the data structure itself.
link: Exists for the purpose of using the linked list pointed by the BUCKET_DESCRIPTOR (pointed by pBucketDesc).
pBucketDesc: points to a BUCKET_DESCRIPTOR that points to the linked list to which the HASH_METADATA belongs.
Used to find the next BUCKET_DESCRIPTOR whose dwLinkNode is non-zero ,
when the bucket specified as an argument to GetNext is the last item in the currently linked list (pointed to by BUCKET_DESCRIPTOR).
*/
struct STATIC_HASH_METADATA
{
	LINKED_NODE link;
	BUCKET_DESCRIPTOR* pBucketDesc = nullptr;
	void* pKey = nullptr;
};

/*
Exists as a callback function for generality of the data structure.
HASH_FUNC: Returns a hash value for the key value specified as an argument.
KEY_ASSIGN_FUNC : Inserts a user-inserted pKey into a pKey that is a member of STATIC_HASH_METADATA.
KEY_COMPARE_FUNC : Used to compare the value of pKey, which is a member of STATIC_HASH_METADATA, with the pKey inserted by the user.
*/
 
typedef DWORD(*HASH_FUNC)(const void*);
typedef void(*KEY_ASSIGN_FUNC)(void**, const void*);
typedef BOOL(*KEY_COMPARE_FUNC)(const void*, const void*);
//typedef LINKED_NODE*(*GET_HASH_LINKED_NODE)(const void* pBucket);

class CStaticHashTable
{
private:
	BUCKET_DESCRIPTOR* pBucketDescriptorTable_ = nullptr;
	DWORD	dwMaxBucketNum_ = 0;
	DWORD   dwKeySize_ = 0;
	DWORD	dwItemNum_ = 0;
	DWORD	dwKeySize = 0;
	HASH_FUNC hashFunc_ = nullptr;
	KEY_ASSIGN_FUNC keyAssignFunc_ = nullptr;
	KEY_COMPARE_FUNC keyCompareFunc_ = nullptr;
	//GET_HASH_LINKED_NODE nodeAddrFindFunc_ = nullptr;

	//Exists for traversal
	//Exists for the purpose of traversing a BUCKET_DESCRIPTOR structure whose dwLinkNode is non - zero.
	//Exists for the linked list that manages the BUCKET_DESCRIPTOR member variable, link.
	LINKED_NODE* pBucketDescHead_ = nullptr;
	LINKED_NODE* pBucketDescTail_ = nullptr;

public:
	BOOL	Initialize(DWORD dwMaxBucketNum, DWORD dwKeySize, HASH_FUNC keyFunc, KEY_ASSIGN_FUNC keyAssignFunc, KEY_COMPARE_FUNC keyCompareFunc, DWORD dwHashMetaDataOffset);
	DWORD	Find(void** ppOutItemList, DWORD dwMaxItemNum, const void* pKeyData, DWORD dwKeySize);
	void	Insert(const void* pItem, const void* pKeyData, DWORD dwSize);
	void	Delete(const void* pBucket);
	DWORD	GetMaxBucketNum() const;
	DWORD	GetAllItems(void** ppOutBucketList, DWORD dwMaxItemNum, BOOL* pbOutInsufficient) const;
	DWORD	GetItemNum() const;
	DWORD	dwHashMetaDataOffset_ = 0;

	// Exists for traversal
	void* GetFirst();
	void* GetNext(const void* pBucket);

	CStaticHashTable();
	~CStaticHashTable();
};
