
template<typename ValType>
class GenericList
{
    public:
        using OnCopyFN = std::function<ValType(ValType)>;
        using OnDestroyFN = std::function<void(ValType)>;



    public:
        static void destroy(GenericList* list)
        {
            if(list != nullptr)
            {
                list->deInit();
                mc_memory_free(list);
                list = nullptr;
            }
        }

        static void destroy(GenericList* list, OnDestroyFN dfn)
        {
            if(list != nullptr)
            {    
                if(dfn != nullptr)
                {
                    clearAndDestroy(list, dfn);
                }
                list->deInit();
                mc_memory_free(list);
            }
        }

        static void clearAndDestroy(GenericList* list, OnDestroyFN dfn)
        {
            size_t i;
            for(i = 0; i < list->count(); i++)
            {
                auto item = list->get(i);
                if(dfn != nullptr)
                {
                    dfn(item);
                }
            }
            list->clear();
        }

        GenericList* copyToHeap(OnCopyFN copyfn, OnDestroyFN dfn)
        {
            bool ok;
            size_t i;
            GenericList* arrcopy;
            (void)ok;
            arrcopy = Memory::make<GenericList<ValType>>(m_listcapacity);
            for(i = 0; i < count(); i++)
            {
                auto item = (ValType)get(i);
                if(copyfn)
                {
                    auto itemcopy = (ValType)copyfn(item);
                    if(!arrcopy->push(itemcopy))
                    {
                        goto listcopyfailed;
                    }
                }
                else
                {
                    if(!arrcopy->push(item))
                    {
                        goto listcopyfailed;
                    }
                }
            }
            return arrcopy;
        listcopyfailed:
            Memory::destroy(arrcopy, dfn);
            return nullptr;
        }

        GenericList* copyToHeap()
        {
            OnCopyFN dummycopy = nullptr;
            OnDestroyFN dummydel = nullptr;
            return copyToHeap(dummycopy, dummydel);
        }

        bool copyToStack(GenericList* dest, OnCopyFN copyfn, OnDestroyFN dfn)
        {
            bool ok;
            size_t i;
            (void)ok;
            (void)dfn;
            for(i = 0; i < count(); i++)
            {
                auto item = (ValType)get(i);
                if(copyfn)
                {
                    auto itemcopy = (ValType)copyfn(item);
                    if(!dest->push(itemcopy))
                    {
                        goto listcopyfailed;
                    }
                }
                else
                {
                    if(!dest->push(item))
                    {
                        goto listcopyfailed;
                    }
                }
            }
            return true;
        listcopyfailed:
            return false;
        }

        bool copyToStack(GenericList* dest)
        {
            OnCopyFN dummycopy = nullptr;
            OnDestroyFN dummydel = nullptr;
            return copyToStack(dest, dummycopy, dummydel);
        }

        GenericList copyToStack(OnCopyFN copyfn, OnDestroyFN dfn)
        {
            GenericList dest;
            copyToStack(&dest, copyfn, dfn);
            return dest;
        }

        GenericList copyToStack()
        {
            GenericList dest;
            copyToStack(&dest);
            return dest;
        }

    public:
        size_t m_listcapacity;
        size_t m_listcount;
        ValType* m_listitems;

    private:
        bool removeAtIntern(unsigned int ix)
        {
            size_t tomovebytes;
            void* src;
            void* dest;
            if(ix == (m_listcount - 1))
            {
                m_listcount--;
                return true;
            }
            tomovebytes = (m_listcount - 1 - ix) * sizeof(ValType);
            dest = m_listitems + (ix * sizeof(ValType));
            src = m_listitems + ((ix + 1) * sizeof(ValType));
            memmove(dest, src, tomovebytes);
            m_listcount--;
            return true;
        }

        void ensureCapacity(size_t needsize, const ValType& fillval, bool first)
        {
            size_t i;
            size_t ncap;
            size_t oldcap;
            (void)first;
            if(m_listcapacity < needsize)
            {
                oldcap = m_listcapacity;
                ncap = MC_UTIL_INCCAPACITY(m_listcapacity + needsize);
                m_listcapacity = ncap;
                if(m_listitems == nullptr)
                {
                    m_listitems = (ValType*)mc_memory_malloc(sizeof(ValType) * ncap);
                }
                else
                {
                    m_listitems = (ValType*)mc_memory_realloc(m_listitems, sizeof(ValType) * ncap);
                }
                for(i = oldcap; i < ncap; i++)
                {
                    m_listitems[i] = fillval;
                }
            }
        }

        template<typename OtherT>
        void moveFrom(OtherT* other)
        {
            m_listcount = other->m_listcount;
            m_listcapacity = other->m_listcapacity;
            m_listitems = other->m_listitems;
        }

    public:
        GenericList(): GenericList(0)
        {
        }

        GenericList(GenericList&& other)
        {
            moveFrom(&other);
        }

        GenericList(const GenericList& other)
        {
            moveFrom(&other);
        }

        GenericList(size_t initialsize)
        {
            m_listcount = 0;
            m_listcapacity = 0;
            m_listitems = nullptr;
            if(initialsize > 0)
            {
                if constexpr(std::is_pointer<ValType>::value)
                {
                    ensureCapacity(initialsize, nullptr, true);
                }
                else
                {
                    ensureCapacity(initialsize, {}, true);                    
                }
            }
        }

        ~GenericList()
        {
            //deInit();
        }

        GenericList& operator=(const GenericList& other)
        {
            moveFrom(&other);
            return *this;
        }

        void orphanData()
        {
            #if 1
            m_listcount = 0;
            m_listcapacity = 0;
            m_listitems = nullptr;
            #endif
        }

        void deInit(OnDestroyFN dfn)
        {
            size_t i;
            for(i=0; i<m_listcount; i++)
            {
                auto item = get(i);
                dfn(item);
            }
            deInit();
        }

        void deInit()
        {
            if(m_listitems != nullptr)
            {
                mc_memory_free(m_listitems);
            }
            m_listitems = nullptr;
            m_listcount = 0;
            m_listcapacity = 0;
        }

        void clear()
        {
            m_listcount = 0;
        }

        size_t count() const
        {
            return m_listcount;
        }

        size_t capacity() const
        {
            return m_listcapacity;
        }

        ValType* data() const
        {
            return m_listitems;
        }

        ValType get(size_t idx) const
        {
            return m_listitems[idx];
        }

        ValType* getp(size_t idx) const
        {
            return &m_listitems[idx];
        }

        ValType top() const
        {
            if(m_listcount == 0)
            {
                if constexpr(std::is_pointer<ValType>::value)
                {
                    return nullptr;
                }
                else
                {
                    return {};
                }
            }
            return get(m_listcount - 1);
        }

        ValType* topp() const
        {
            int ofs = 0;
            if(m_listcount == 0)
            {
                return nullptr;
            }
            if(m_listcount > 0)
            {
                ofs = m_listcount - 1;
            }
            return getp(ofs);
        }


        ValType* set(size_t idx, const ValType& val)
        {
            size_t need;
            need = idx + 1;
            if(((idx == 0) || (m_listcapacity == 0)) || (idx >= m_listcapacity))
            {
                if constexpr(std::is_pointer<ValType>::value)
                {
                    ensureCapacity(need, nullptr, false);
                }
                else
                {
                    ensureCapacity(need, {}, false);
                }
            }
            if(idx > m_listcount)
            {
                m_listcount = idx;
            }
            m_listitems[idx] = val;
            return &m_listitems[idx];
        }

        bool push(const ValType& value)
        {
            size_t oldcap;
            if(m_listcapacity < m_listcount + 1)
            {
                oldcap = m_listcapacity;
                m_listcapacity = MC_UTIL_INCCAPACITY(oldcap);
                if(m_listitems == nullptr)
                {
                    m_listitems = (ValType*)mc_memory_malloc(sizeof(ValType) * m_listcapacity);
                }
                else
                {
                    m_listitems = (ValType*)mc_memory_realloc(m_listitems, sizeof(ValType) * m_listcapacity);
                }
            }
            m_listitems[m_listcount] = value;
            m_listcount++;
            return true;
        }

        bool pop(ValType* dest)
        {
            if(m_listcount > 0)
            {
                if(dest != nullptr)
                {
                    *dest = m_listitems[m_listcount - 1];
                }
                m_listcount--;
                return true;
            }
            return false;
        }

        bool removeAt(unsigned int ix)
        {
            if(ix >= m_listcount)
            {
                return false;
            }
            if(ix == 0)
            {
                m_listitems += sizeof(ValType);
                m_listcapacity--;
                m_listcount--;
                return true;
            }
            return removeAtIntern(ix);
        }

        void setEmpty()
        {
            if((m_listcapacity > 0) && (m_listitems != nullptr))
            {
                memset(m_listitems, 0, sizeof(ValType) * m_listcapacity);
            }
            m_listcount = 0;
            m_listcapacity = 0;
        }
};


