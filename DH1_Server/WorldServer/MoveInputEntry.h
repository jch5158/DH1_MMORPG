#pragma once

struct MoveInputEntry
{
	uint64 mAccountId = 0;
	uint32 mSequenceId = 0;
	float mDirectionX = 0.0f;
	float mDirectionY = 0.0f;
	float mDirectionZ = 0.0f;
	float mRotationYaw = 0.0f;
	int64 mClientTimestamp = 0;
	float mPositionZ = 0.0f;
};
