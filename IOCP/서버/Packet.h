#pragma once

class PacketData
{
public:
	UINT32 SessionIndex = 0;
	UINT32 DataSize = 0;
	std::shared_ptr<char> pPacketData;

	PacketData()
	{
		pPacketData = nullptr;
	}

	PacketData(UINT32 sessionIndex_, UINT32 dataSize_, std::shared_ptr<char>& pData_)
	{
		SessionIndex = sessionIndex_;
		DataSize = dataSize_;
		pPacketData = pData_;
	}
	~PacketData() { pPacketData.reset(); }
};