// Copyright 2017 Isaac Hsu. MIT License

#pragma once

#include "Array.h"

template<typename ElementType, typename Allocator>
class TCyclicBuffer;

template<typename ContainerType, typename ElementType>
class TCyclicBufferIterator
{
	typedef TCyclicBufferIterator ThisClass;

public:

	TCyclicBufferIterator(ContainerType& InContainer)
		: Container(InContainer)
		, Index(InContainer.GetHeadIndex())
	{}

	TCyclicBufferIterator(ContainerType& InContainer, int32 StartIndex)
		: Container(InContainer)
		, Index(InContainer.GetHeadIndex() + StartIndex)
	{
		if (Container.IsValidIndex(StartIndex))
		{
			if (Index >= Container.Num())
			{
				Index -= Container.Num();
			}
		}
		else
		{
			Index = Container.Num();
		}
	}

	/** Advances iterator to the next element in the container. */
	ThisClass& operator++()
	{
		Advance();
		return *this;
	}

	ThisClass operator++(int)
	{
		ThisClass Tmp(*this);
		Advance();
		return Tmp;
	}

	/** @name Element access */
	//@{
	ElementType& operator* () const
	{
		return Container[Index];
	}

	ElementType* operator-> () const
	{
		return &Container[Index];
	}
	//@}

	/** conversion to "bool" returning true if the iterator has not reached the last element. */
	FORCEINLINE explicit operator bool() const
	{
		return Container.IsValidIndex(Index);
	}
	/** inverse of the "bool" operator */
	FORCEINLINE bool operator !() const
	{
		return !(bool)*this;
	}

private:

	void Advance()
	{
		if (Index == Container.TailIndex) // If we have reached the tail
		{
			Index = Container.Num();
		}
		else
		{
			++Index;
			int32 Limit = Container.Num() - 1;
			if (Index > Limit && Container.TailIndex != Limit) // If we have reached the upper bound but not the tail
			{
				Index = 0;
			}
		}
	}

	int32 Index;
	ContainerType& Container;
};

template<typename ContainerType, typename ElementType>
class TCyclicBufferReverseIterator
{
	typedef TCyclicBufferReverseIterator ThisClass;

public:

	TCyclicBufferReverseIterator(ContainerType& InContainer)
		: Container(InContainer)
		, Index(InContainer.TailIndex)
	{}

	TCyclicBufferReverseIterator(ContainerType& InContainer, int32 StartIndex)
		: Container(InContainer)
		, Index(InContainer.TailIndex - StartIndex)
	{
		if (Container.IsValidIndex(StartIndex))
		{
			if (Index < 0)
			{
				Index += Container.Num();
			}
		}
		else
		{
			Index = INDEX_NONE;
		}
	}

	/** Advances iterator to the next element in the container. */
	ThisClass& operator++()
	{
		Advance();
		return *this;
	}

	ThisClass operator++(int)
	{
		ThisClass Tmp(*this);
		Advance();
		return Tmp;
	}

	/** @name Element access */
	//@{
	ElementType& operator* () const
	{
		return Container[Index];
	}

	ElementType* operator-> () const
	{
		return &Container[Index];
	}
	//@}

	/** conversion to "bool" returning true if the iterator has not reached the last element. */
	FORCEINLINE explicit operator bool() const
	{
		return Container.IsValidIndex(Index);
	}
	/** inverse of the "bool" operator */
	FORCEINLINE bool operator !() const
	{
		return !(bool)*this;
	}

private:

	void Advance()
	{
		if (Index == Container.GetHeadIndex()) // If we have reached the head
		{
			Index = INDEX_NONE;
		}
		else
		{
			--Index;
			int32 Limit = Container.Num() - 1;
			if (Index < 0 && Container.TailIndex != Limit) // If we have reached the lower bound but not the head
			{
				Index = Limit;
			}
		}
	}

	int32 Index;
	ContainerType& Container;
};

/**
* Homogeneous cyclic buffer based on TArray. When a new element is added, the oldest one may be replaced if the buffer is full.
*
* Caution: Must resize the buffer before adding elements to it.
*/
template<typename ElementType, typename Allocator = FDefaultAllocator>
class TCyclicBuffer : private TArray<ElementType, Allocator>
{
	typedef TArray<ElementType, Allocator> Super;

public:

	typedef TCyclicBufferIterator<TCyclicBuffer, ElementType> TIterator;
	friend TIterator;

	typedef TCyclicBufferIterator<const TCyclicBuffer, const ElementType> TConstIterator;
	friend TConstIterator;

	typedef TCyclicBufferReverseIterator<TCyclicBuffer, ElementType> TReverseIterator;
	friend TReverseIterator;

	typedef TCyclicBufferReverseIterator<const TCyclicBuffer, const ElementType> TConstReverseIterator;
	friend TConstReverseIterator;

public:

	TCyclicBuffer()	: TailIndex(INDEX_NONE) {}

	using Super::Num;
	using Super::Max;
	using Super::GetData;

	/**
	* Returns n-th last element from the buffer.
	*
	* @param IndexFromTheEnd (Optional) Index from the end of buffer (default = 0).
	* @returns Reference to n-th last element from the buffer.
	*/
	FORCEINLINE ElementType* LastOrNull(int32 IndexFromTheEnd = 0)
	{
		ElementType* Result = nullptr;
		if (IndexFromTheEnd >= 0 && IndexFromTheEnd < Super::ArrayNum)
		{
			int32 Index = TailIndex - IndexFromTheEnd;
			if (Index < 0)
			{
				Index += Super::ArrayNum;
			}
			Result = GetData() + Index;
		}
		return Result;
	}

	/**
	* Returns n-th last element from the buffer.
	*
	* Const version of the above.
	*
	* @param IndexFromTheEnd (Optional) Index from the end of buffer (default = 0).
	* @returns Reference to n-th last element from the buffer.
	*/
	FORCEINLINE const ElementType* LastOrNull(int32 IndexFromTheEnd = 0) const
	{
		const ElementType* Result = nullptr;
		if (IndexFromTheEnd >= 0 && IndexFromTheEnd < Super::ArrayNum)
		{
			int32 Index = TailIndex - IndexFromTheEnd;
			if (Index < 0)
			{
				Index += Super::ArrayNum;
			}
			Result = GetData() + Index;
		}
		return Result;
	}

	/**
	* Empties the array. It calls the destructors on held items if needed.
	*
	* @param Slack (Optional) The expected usage size after empty operation. Default is 0.
	*/
	FORCEINLINE void Reset(int32 Slack = 0)
	{
		Super::Reset(Slack);
		TailIndex = INDEX_NONE;
	}

	/**
	* Adds a new item to the buffer, possibly replacing the oldest element.
	*
	* @param Item The item to add
	* @return Index to the new item
	*/
	int32 Add(const ElementType& Item)
	{
		if (Super::ArrayMax == 0)
		{
			return INDEX_NONE;
		}

		if (Super::ArrayNum < Super::ArrayMax)
		{
			TailIndex = Super::Add(Item);
		}
		else
		{
			int32 HeadIndex = (TailIndex < Super::ArrayMax - 1) ? TailIndex + 1 : 0;
			(*this)[HeadIndex] = Item;
			TailIndex = HeadIndex;
		}

		return TailIndex;
	}

	/**
	* Creates a iterator for the contents of this buffer
	*
	* @param StartIndex (Optional) Index from the beginning of buffer (default = 0).
	* @returns The iterator.
	*/
	TIterator CreateIterator(int32 StartIndex = 0)
	{
		return TIterator(*this, StartIndex);
	}

	/**
	* Creates a const iterator for the contents of this buffer
	*
	* @returns The const iterator.
	* @param StartIndex (Optional) Index from the beginning of buffer (default = 0).
	*/
	TConstIterator CreateConstIterator(int32 StartIndex = 0) const
	{
		return TConstIterator(*this, StartIndex);
	}

	/**
	* Creates a reverse iterator for the contents of this buffer
	*
	* @param StartIndex (Optional) Index from the end of buffer (default = 0).
	* @returns The iterator.
	*/
	TReverseIterator CreateReverseIterator(int32 StartIndex = 0)
	{
		return TReverseIterator(*this, StartIndex);
	}

	/**
	* Creates a const reverse iterator for the contents of this buffer
	*
	* @param StartIndex (Optional) Index from the end of buffer (default = 0).
	* @returns The const iterator.
	*/
	TConstReverseIterator CreateConstReverseIterator(int32 StartIndex = 0) const
	{
		return TConstReverseIterator(*this, StartIndex);
	}

protected:

	/* The index of the first element. */
	int32 GetHeadIndex() const 
	{ 
		if (Super::ArrayNum == 0)
		{
			return INDEX_NONE;
		}
		//else if (Super::ArrayNum < Super::ArrayMax)
		//{
		//	return 0;
		//}
		else
		{
			return (TailIndex < Super::ArrayNum - 1) ? TailIndex + 1 : 0;
		}
	}

	/* The index of the last element. */
	int32 TailIndex;
};