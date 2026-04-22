#pragma once

namespace qos {

enum class SenderPressureState { None, Warning, Congested };

inline const char* senderPressureStateStr(SenderPressureState state)
{
	switch (state) {
		case SenderPressureState::None: return "none";
		case SenderPressureState::Warning: return "warning";
		case SenderPressureState::Congested: return "congested";
	}

	return "none";
}

inline bool senderPressureActive(SenderPressureState state)
{
	return state != SenderPressureState::None;
}

inline bool senderPressureCongested(SenderPressureState state)
{
	return state == SenderPressureState::Congested;
}

} // namespace qos
