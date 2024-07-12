#pragma once
#include <set>
class DisIdPool
{
	enum num
	{
		END = -1,
	};
	using MydisType = std::set<unsigned>;
	using MyIter = std::set<unsigned>::iterator;

public:
	__forceinline bool isDeletedSession(unsigned id)
	{
		return disconSessionSet.find(id) != disconSessionSet.end();
	}
	__forceinline unsigned GetSize()
	{
		return disconSessionSet.size();
	}
	unsigned GetFirst();
	unsigned GetNext(unsigned id);

	__forceinline void disconnect(unsigned id)
	{
		disconSessionSet.insert(id);
	}

private:
	MydisType disconSessionSet;
};
