#pragma once

enum EMissionObjectiveState : int
{
	EMOS_MIN				= 0,

	EMOS_WAITING		= 0,
	EMOS_RECEIVED		= 1,
	EMOS_COMPLETED	= 2,
	EMOS_FAILED			= 3,

	EMOS_MAX				= 3,
};


