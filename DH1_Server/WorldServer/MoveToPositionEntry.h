#pragma once

struct MoveToPositionEntry
{
	uint64 mAccountId = 0;
	uint32 mSequenceId = 0;
	float mDestinationX = 0.0f;
	float mDestinationY = 0.0f;
	float mDestinationZ = 0.0f;
	int64 mClientTimestamp = 0;
	float mCurrentX = 0.0f;
	float mCurrentY = 0.0f;
	float mCurrentZ = 0.0f;
	bool mHasCurrentPosition = false;
};
