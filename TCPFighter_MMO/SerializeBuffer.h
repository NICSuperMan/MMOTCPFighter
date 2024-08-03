#pragma once
#define IN
#define OUT
#include <memory.h>
class SerializeBuffer
{
public:
	enum RING_SIZE
	{
		RINGBUFFER_SIZE = 10000
	};

	enum PACKET_SIZE
	{
		DEFAULT_SIZE = RINGBUFFER_SIZE / 8
	};

	SerializeBuffer()
		:bufferSize_{ 0 }, front_{ 0 }, rear_{ 0 }, pBuffer_{ nullptr }
	{}

	void AllocBuffer(IN const int size = DEFAULT_SIZE)
	{
		pBuffer_ = new char[size];
		bufferSize_ = size;
	}
	void FreeBuffer(void)
	{
		delete[] pBuffer_;
		pBuffer_ = nullptr;
	}

	void Resize(void)
	{
		if (bufferSize_ * 2 > RINGBUFFER_SIZE)
		{
			throw 1;
		}
		char* pTempBuffer = new char[bufferSize_ * 2];
		memcpy_s(pTempBuffer, bufferSize_ * 2, pBuffer_, bufferSize_);
		bufferSize_ *= 2;
		delete[] pBuffer_;
		pBuffer_ = pTempBuffer;
	}

	void Clear(void)
	{
		front_ = rear_ = 0;
	}
	int GetBufferSize(void)
	{
		return bufferSize_;
	}

	int GetData(OUT char* pDest, IN int sizeToGet)
	{
		if (rear_ - front_ < sizeToGet)
		{
			return 0;
		}
		else
		{
			memcpy_s(pDest, sizeToGet, pBuffer_ + front_, sizeToGet);
			front_ += sizeToGet;
			return sizeToGet;
		}
	}

	int PutData(IN char* pSrc, IN int sizeToPut)
	{
		if (bufferSize_ - rear_ < sizeToPut)
		{
			Resize();
		}
		memcpy_s(pBuffer_ + rear_, sizeToPut, pSrc, sizeToPut);
		rear_ += sizeToPut;
		return sizeToPut;
	}

	int GetUsedDataSize(void)
	{
		return rear_ - front_;
	}

	char* GetBufferPtr(void)
	{
		return pBuffer_;
	}

	int MoveWritePos(IN int sizeToWrite)
	{
		rear_ += sizeToWrite;
		return sizeToWrite;
	}

	int MoveReadPos(IN int sizeToRead)
	{
		front_ += sizeToRead;
		return sizeToRead;
	}
	char* pBuffer_;
	int bufferSize_;
	int front_;
	int rear_;

	SerializeBuffer& operator <<(IN const unsigned char value)
	{
		if (bufferSize_ - rear_ < sizeof(value))
		{
			Resize();
		}
		*(unsigned char*)(pBuffer_ + rear_) = value;
		rear_ += sizeof(value);
		return *this;
	}
	SerializeBuffer& operator >>(OUT unsigned char& value)
	{
		// value의 size만큼 읽을게 없음.
		if (rear_ - front_ < sizeof(value))
		{
			throw 1;
			//return *this;
		}
		value = *(unsigned char*)(pBuffer_ + front_);
		front_ += sizeof(value);
		return *this;
	}

	SerializeBuffer& operator <<(IN const char value)
	{
		if (bufferSize_ - rear_ < sizeof(value))
		{
			Resize();
		}
		*(char*)(pBuffer_ + rear_) = value;
		rear_ += sizeof(value);
		return *this;
	}
	SerializeBuffer& operator >>(OUT char& value)
	{
		if (rear_ - front_ < sizeof(value))
		{
			throw 1;
			//return *this;
		}
		value = *(char*)(pBuffer_ + front_);
		front_ += sizeof(value);
		return *this;
	}

	SerializeBuffer& operator <<(IN const short value)
	{
		if (bufferSize_ - rear_ < sizeof(value))
		{
			Resize();
		}
		*(short*)(pBuffer_ + rear_) = value;
		rear_ += sizeof(value);
		return *this;
	}
	SerializeBuffer& operator >>(OUT short& value)
	{
		if (rear_ - front_ < sizeof(value))
		{
			throw 1;
			//return *this;
		}
		value = *(short*)(pBuffer_ + front_);
		front_ += sizeof(value);
		return *this;
	}

	SerializeBuffer& operator <<(IN const unsigned short value)
	{
		if (bufferSize_ - rear_ < sizeof(value))
		{
			Resize();
		}
		*(unsigned short*)(pBuffer_ + rear_) = value;
		rear_ += sizeof(value);
		return *this;
	}
	SerializeBuffer& operator >>(OUT unsigned short& value)
	{
		if (rear_ - front_ < sizeof(value))
		{
			throw 1;
			//return *this;
		}
		value = *(unsigned short*)(pBuffer_ + front_);
		front_ += sizeof(value);
		return *this;
	}

	SerializeBuffer& operator <<(IN const int value)
	{
		if (bufferSize_ - rear_ < sizeof(value))
		{
			Resize();
		}
		*(int*)(pBuffer_ + rear_) = value;
		rear_ += sizeof(value);
		return *this;
	}
	SerializeBuffer& operator >>(OUT int& value)
	{
		if (rear_ - front_ < sizeof(value))
		{
			throw 1;
			//return *this;
		}
		value = *(int*)(pBuffer_ + front_);
		front_ += sizeof(value);
		return *this;
	}

	SerializeBuffer& operator <<(IN const unsigned int value)
	{
		if (bufferSize_ - rear_ < sizeof(value))
		{
			Resize();
		}
		*(unsigned int*)(pBuffer_ + rear_) = value;
		rear_ += sizeof(value);
		return *this;
	}
	SerializeBuffer& operator >>(OUT unsigned int& value)
	{
		if (rear_ - front_ < sizeof(value))
		{
			throw 1;
			//return *this;
		}
		value = *(unsigned int*)(pBuffer_ + front_);
		front_ += sizeof(value);
		return *this;
	}

	SerializeBuffer& operator <<(IN const long value)
	{
		if (bufferSize_ - rear_ < sizeof(value))
		{
			Resize();
		}
		*(long*)(pBuffer_ + rear_) = value;
		rear_ += sizeof(value);
		return *this;
	}

	SerializeBuffer& operator >>(OUT long& value)
	{
		if (rear_ - front_ < sizeof(value))
		{
			throw 1;
			//return *this;
		}
		value = *(long*)(pBuffer_ + front_);
		front_ += sizeof(value);
		return *this;
	}

	SerializeBuffer& operator <<(IN const unsigned long value)
	{
		if (bufferSize_ - rear_ < sizeof(value))
		{
			Resize();
		}
		*(unsigned long*)(pBuffer_ + rear_) = value;
		rear_ += sizeof(value);
		return *this;
	}
	SerializeBuffer& operator >>(OUT unsigned long& value)
	{
		if (rear_ - front_ < sizeof(value))
		{
			throw 1;
			//return *this;
		}
		value = *(unsigned long*)(pBuffer_ + front_);
		front_ += sizeof(value);
		return *this;
	}

	SerializeBuffer& operator <<(IN const __int64 value)
	{
		if (bufferSize_ - rear_ < sizeof(value))
		{
			Resize();
		}
		*(__int64*)(pBuffer_ + rear_) = value;
		rear_ += sizeof(value);
		return *this;
	}

	SerializeBuffer& operator >>(OUT __int64& value)
	{
		if (rear_ - front_ < sizeof(value))
		{
			throw 1;
			//return *this;
		}
		value = *(__int64*)(pBuffer_ + front_);
		front_ += sizeof(value);
		return *this;
	}
	//
	SerializeBuffer& operator <<(IN const unsigned __int64 value)
	{
		if (bufferSize_ - rear_ < sizeof(value))
		{
			Resize();
		}
		*(unsigned __int64*)(pBuffer_ + rear_) = value;
		rear_ += sizeof(value);
		return *this;
	}
	SerializeBuffer& operator >>(OUT unsigned __int64& value)
	{
		if (rear_ - front_ < sizeof(value))
		{
			throw 1;
			//return *this;
		}
		value = *(unsigned __int64*)(pBuffer_ + front_);
		front_ += sizeof(value);
		return *this;
	}

	SerializeBuffer& operator <<(IN const float value)
	{
		if (bufferSize_ - rear_ < sizeof(value))
		{
			Resize();
		}
		*(float*)(pBuffer_ + rear_) = value;
		rear_ += sizeof(value);
		return *this;
	}
	SerializeBuffer& operator >>(OUT float& value)
	{
		if (rear_ - front_ < sizeof(value))
		{
			throw 1;
			//return *this;
		}
		value = *(float*)(pBuffer_ + front_);
		front_ += sizeof(value);
		return *this;
	}

	SerializeBuffer& operator <<(IN const double value)
	{
		if (bufferSize_ - rear_ < sizeof(value))
		{
			Resize();
		}
		*(double*)(pBuffer_ + rear_) = value;
		rear_ += sizeof(value);
		return *this;
	}

	SerializeBuffer& operator >>(OUT double& value)
	{
		if (rear_ - front_ < sizeof(value))
		{
			throw 1;
			//return *this;
		}
		value = *(double*)(pBuffer_ + front_);
		front_ += sizeof(value);
		return *this;
	}
};
