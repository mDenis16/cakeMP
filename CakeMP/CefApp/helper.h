#pragma once
static LARGE_INTEGER qi_freq_ = {};
inline uint64_t time_now()
{
	if (!qi_freq_.HighPart && !qi_freq_.LowPart) {
		QueryPerformanceFrequency(&qi_freq_);
	}
	LARGE_INTEGER t = {};
	QueryPerformanceCounter(&t);
	return static_cast<uint64_t>(
		(t.QuadPart / double(qi_freq_.QuadPart)) * 1000000);
}
inline void log_message(const char* msg, ...) { printf(msg); }

template<class T>
std::shared_ptr<T> to_com_ptr(T* obj)
{
	return std::shared_ptr<T>(obj, [](T* p) { if (p) p->Release(); });
}
 enum class MouseButton
{
	Left,
	Middle,
	Right
};