#pragma once

#include "Define.h"

class myQueue
{
public:
	
	myQueue()
	{
		Head = nullptr;
		Tail = nullptr;
		mSize = 0;
	}

	~myQueue()
	{
		while (!empty())
		{
			pop();
		}
	}

	void push(PacketData* pData_)
	{
		if (empty())
		{
			Head = new Node(pData_);
			Tail = Head;
			mSize++;
		}
		else
		{
			Tail->next = new Node(pData_);
			Tail = Tail->next;
			mSize++;
		}
	}

	void pop()
	{
		Node* tmp = Head;
		Head = Head->next;
		delete tmp;
		mSize--;
	}

	PacketData* front() { return Head->data; }

	bool empty() const { return mSize == 0; }
	size_t size() { return mSize; }

private:
	class Node
	{
	public:
		Node(PacketData* pData_) : data(pData_) { next = nullptr; }
		~Node() {}

		Node* next;
		PacketData* data;
	};

	Node* Head;
	Node* Tail;
	size_t mSize;
};