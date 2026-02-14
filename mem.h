
#ifndef __libmc_mem_h__
#define __libmc_mem_h__

#include <iostream>
#include <type_traits>
#include <concepts>

void* mc_memory_malloc(size_t sz);
void* mc_memory_realloc(void* p, size_t nsz);
void* mc_memory_calloc(size_t count, size_t typsize);
void mc_memory_free(void* ptr);

template <typename ClassT>
concept MemoryClassHasDestroyFunc = requires(ClassT* ptr) {
    { ClassT::destroy(ptr) } -> std::same_as<void>;
};


class Memory
{
    public:
        template<typename ClassT, typename... ArgsT>
        static inline ClassT* make(ArgsT&&... args)
        {
            ClassT* tmp;
            ClassT* ret;
            tmp = (ClassT*)mc_memory_malloc(sizeof(ClassT));
            ret = new(tmp) ClassT(args...);
            return ret;
        }

        template<typename ClassT, typename... ArgsT>
        static inline void destroy(ClassT* cls, ArgsT&&... args)
        {
            if constexpr (MemoryClassHasDestroyFunc<ClassT>)
            {
                //std::cerr << "Memory::destroy: using destroy" << std::endl;
                ClassT::destroy(cls, args...);
            }
            else
            {
                //std::cerr << "Memory::destroy: using free()" << std::endl;
                mc_memory_free(cls);
            }
        }
};


#endif /* __libmc_mem_h__ */
