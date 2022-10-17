#pragma once
#include "iostream"
//This was based of of the Handles that Sam boots used in a previous renderer of his
namespace FVR
{
	
	typedef struct Handle MeshHandle;
	typedef struct Handle TextureHandle;
	typedef struct Handle QuadHandle;
	struct Handle
	{
		uint32_t INVALID = UINT32_MAX;

		Handle() {};
		Handle(const uint32_t handle) { mHandle = handle; }
		~Handle() {};

		bool operator ==(const Handle& A) const
		{
			return mHandle == A.mHandle;
		}

		operator uint32_t() const
		{
			return mHandle;
		};

		bool IsValid() const
		{
			return mHandle != INVALID;
		}
		
	private:
		uint32_t mHandle = INVALID;
	};
}
namespace std
{
	template <>
	struct hash<FVR::Handle>
	{
		std::size_t operator()(const FVR::Handle& k) const
		{
			using std::size_t;
			using std::hash;

			return hash<uint32_t>()(k);
		}
	};
}