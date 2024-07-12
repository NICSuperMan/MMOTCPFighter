#include "disPool.h"

unsigned DisIdPool::GetFirst()
{
    MyIter iter = disconSessionSet.begin();
    if (iter == disconSessionSet.end())
    {
        return (unsigned)DisIdPool::END;
    }
    return *iter;
}

unsigned DisIdPool::GetNext(unsigned id)
{
    MyIter iter = disconSessionSet.find(id);
    ++iter;
    if (iter == disconSessionSet.end())
    {
        return (unsigned)DisIdPool::END;
    }
    return *iter;
}

