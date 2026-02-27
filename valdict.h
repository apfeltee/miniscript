
template<typename KeyType, typename ValType>
class ValDict
{
    public:
        enum
        {
            VDInvalidIndex = (UINT_MAX),
        };

    public:
        static inline bool initDict(ValDict* dict, unsigned int initialcapacity)
        {
            unsigned int i;
            dict->m_vdcells = nullptr;
            dict->m_vdkeys = nullptr;
            dict->m_vdvalues = nullptr;
            dict->m_vdcellindices = nullptr;
            dict->m_vdhashes = nullptr;
            dict->m_vdcount = 0;
            dict->m_vdcellcapacity = initialcapacity;
            dict->m_vditemcapacity = (unsigned int)(initialcapacity * 0.7f);
            dict->m_funckeyequalsfn = nullptr;
            dict->m_funchashfn = nullptr;
            dict->m_vdcells = (unsigned int*)mc_memory_malloc(dict->m_vdcellcapacity * sizeof(unsigned int));
            dict->m_vdkeys = (char**)mc_memory_malloc(dict->m_vditemcapacity * sizeof(KeyType));
            dict->m_vdvalues = (void**)mc_memory_malloc(dict->m_vditemcapacity * sizeof(ValType));
            dict->m_vdcellindices = (unsigned int*)mc_memory_malloc(dict->m_vditemcapacity * sizeof(unsigned int));
            dict->m_vdhashes = (long unsigned int*)mc_memory_malloc(dict->m_vditemcapacity * sizeof(unsigned long));
            if(dict->m_vdcells == nullptr || dict->m_vdkeys == nullptr || dict->m_vdvalues == nullptr || dict->m_vdcellindices == nullptr || dict->m_vdhashes == nullptr)
            {
                goto dictallocfailed;
            }
            for(i = 0; i < dict->m_vdcellcapacity; i++)
            {
                dict->m_vdcells[i] = VDInvalidIndex;
            }
            return true;
        dictallocfailed:
            mc_memory_free(dict->m_vdcells);
            mc_memory_free(dict->m_vdkeys);
            mc_memory_free(dict->m_vdvalues);
            mc_memory_free(dict->m_vdcellindices);
            mc_memory_free(dict->m_vdhashes);
            return false;
        }

        static inline void deinit(ValDict* dict)
        {
            dict->m_vdcount = 0;
            dict->m_vditemcapacity = 0;
            dict->m_vdcellcapacity = 0;
            mc_memory_free(dict->m_vdcells);
            mc_memory_free(dict->m_vdkeys);
            mc_memory_free(dict->m_vdvalues);
            mc_memory_free(dict->m_vdcellindices);
            mc_memory_free(dict->m_vdhashes);
            dict->m_vdcells = nullptr;
            dict->m_vdkeys = nullptr;
            dict->m_vdvalues = nullptr;
            dict->m_vdcellindices = nullptr;
            dict->m_vdhashes = nullptr;
        }


        static inline void destroy(ValDict* dict)
        {
            if(dict != nullptr)
            {
                ValDict::deinit(dict);
                mc_memory_free(dict);
            }
        }

    public:
        unsigned int* m_vdcells;
        unsigned long* m_vdhashes;
        char** m_vdkeys;
        void** m_vdvalues;
        unsigned int* m_vdcellindices;
        unsigned int m_vdcount;
        unsigned int m_vditemcapacity;
        unsigned int m_vdcellcapacity;
        mcitemhashfn_t m_funchashfn;
        mcitemcomparefn_t m_funckeyequalsfn;

    public:
        inline ValDict(): ValDict(StrDict::DefaultInitSize)
        {
        }

        inline ValDict(unsigned int mincapacity)
        {
            bool ok;
            unsigned int capacity;
            (void)ok;
            capacity = mc_util_upperpowoftwo(mincapacity * 2);
            ok = ValDict::initDict(this, capacity);
            MC_ASSERT(ok);
        }

        inline void setHashFunction(mcitemhashfn_t hashfn)
        {
            m_funchashfn = hashfn;
        }

        inline void setEqualsFunction(mcitemcomparefn_t equalsfn)
        {
            m_funckeyequalsfn = equalsfn;
        }

        inline bool setKVIntern(unsigned int cellix, unsigned long hash, void* key, void* value)
        {
            bool ok;
            bool found;
            unsigned int lastix;
            (void)ok;
            if(m_vdcount >= m_vditemcapacity)
            {
                ok = growAndRehash();
                cellix = getCellIndex(key, hash, &found);
            }
            lastix = m_vdcount;
            m_vdcount++;
            m_vdcells[cellix] = lastix;
            setKeyAt(lastix, key);
            setValueAt(lastix, value);
            m_vdcellindices[lastix] = cellix;
            m_vdhashes[lastix] = hash;
            return true;
        }

        inline bool setKV(void* key, void* value)
        {
            bool found;
            unsigned long hash;
            unsigned int cellix;
            unsigned int itemix;
            hash = hashKey(key);
            found = false;
            cellix = getCellIndex(key, hash, &found);
            if(found)
            {
                itemix = m_vdcells[cellix];
                setValueAt(itemix, value);
                return true;
            }
            return setKVIntern(cellix, hash, key, value);
        }

        inline void* get(void* key)
        {
            bool found;
            unsigned int itemix;
            unsigned long hash;
            unsigned long cellix;
            if(m_vdcount == 0)
            {
                return nullptr;
            }
            hash = hashKey(key);
            found = false;
            cellix = getCellIndex(key, hash, &found);
            if(!found)
            {
                return nullptr;
            }
            itemix = m_vdcells[cellix];
            return getValueAt(itemix);
        }

        inline KeyType* getKeyAt(unsigned int ix)
        {
            if(ix >= m_vdcount)
            {
                return nullptr;
            }
            return (KeyType*)((char*)m_vdkeys + (sizeof(KeyType) * ix));
        }

        inline ValType* getValueAt(unsigned int ix)
        {
            if(ix >= m_vdcount)
            {
                return nullptr;
            }
            return (ValType*)((char*)m_vdvalues + (sizeof(ValType) * ix));
        }

        inline unsigned int getCapacity()
        {
            return m_vditemcapacity;
        }

        template<typename InputValT>
        inline bool setValueAt(unsigned int ix, InputValT value)
        {
            size_t offset;
            if(ix >= m_vdcount)
            {
                return false;
            }
            offset = ix * sizeof(ValType);
            memcpy((char*)m_vdvalues + offset, value, sizeof(ValType));
            return true;
        }

        inline int count()
        {
            return m_vdcount;
        }

        inline bool removeByKey(void* key)
        {
            bool found;
            unsigned int x;
            unsigned int k;
            unsigned int i;
            unsigned int j;
            unsigned int cell;
            unsigned int itemix;
            unsigned int lastitemix;
            unsigned long hash;
            void* lastkey;
            void* lastvalue;
            hash = hashKey(key);
            found = false;
            cell = getCellIndex(key, hash, &found);
            if(!found)
            {
                return false;
            }
            itemix = m_vdcells[cell];
            lastitemix = m_vdcount - 1;
            if(itemix < lastitemix)
            {
                lastkey = getKeyAt(lastitemix);
                setKeyAt(itemix, lastkey);
                lastvalue = getKeyAt(lastitemix);
                setValueAt(itemix, lastvalue);
                m_vdcellindices[itemix] = m_vdcellindices[lastitemix];
                m_vdhashes[itemix] = m_vdhashes[lastitemix];
                m_vdcells[m_vdcellindices[itemix]] = itemix;
            }
            m_vdcount--;
            i = cell;
            j = i;
            for(x = 0; x < (m_vdcellcapacity - 1); x++)
            {
                j = (j + 1) & (m_vdcellcapacity - 1);
                if(m_vdcells[j] == VDInvalidIndex)
                {
                    break;
                }
                k = (unsigned int)(m_vdhashes[m_vdcells[j]]) & (m_vdcellcapacity - 1);
                if((j > i && (k <= i || k > j)) || (j < i && (k <= i && k > j)))
                {
                    m_vdcellindices[m_vdcells[j]] = i;
                    m_vdcells[i] = m_vdcells[j];
                    i = j;
                }
            }
            m_vdcells[i] = VDInvalidIndex;
            return true;
        }

        inline void clear()
        {
            unsigned int i;
            m_vdcount = 0;
            for(i = 0; i < m_vdcellcapacity; i++)
            {
                m_vdcells[i] = VDInvalidIndex;
            }
        }

        inline unsigned int getCellIndex(void* key, unsigned long hash, bool* outfound)
        {
            bool areequal;
            unsigned int i;
            unsigned int ix;
            unsigned int cell;
            unsigned int cellix;
            unsigned long hashtocheck;
            void* keytocheck;
            *outfound = false;
            cellix = (unsigned int)hash & (m_vdcellcapacity - 1);
            for(i = 0; i < m_vdcellcapacity; i++)
            {
                ix = (cellix + i) & (m_vdcellcapacity - 1);
                cell = m_vdcells[ix];
                if(cell == VDInvalidIndex)
                {
                    return ix;
                }
                hashtocheck = m_vdhashes[cell];
                if(hash != hashtocheck)
                {
                    continue;
                }
                keytocheck = getKeyAt(cell);
                areequal = keysAreEqual(key, keytocheck);
                if(areequal)
                {
                    *outfound = true;
                    return ix;
                }
            }
            return VDInvalidIndex;
        }

        inline bool growAndRehash()
        {
            bool ok;
            unsigned int i;
            unsigned ncap;
            char* key;
            void* value;
            (void)ok;
            ncap = MC_UTIL_INCCAPACITY(m_vdcellcapacity);    
            ValDict newdict(ncap);
            newdict.m_funckeyequalsfn = m_funckeyequalsfn;
            newdict.m_funchashfn = m_funchashfn;
            for(i = 0; i < m_vdcount; i++)
            {
                key = (char*)getKeyAt(i);
                value = getValueAt(i);
                ok = newdict.setKV(key, value);
            }
            ValDict::deinit(this);
            *this = newdict;
            return true;
        }

        template<typename InputKeyT>
        inline bool setKeyAt(unsigned int ix, InputKeyT key)
        {
            size_t offset;
            if(ix >= m_vdcount)
            {
                return false;
            }
            offset = ix * sizeof(KeyType);
            memcpy((char*)m_vdkeys + offset, key, sizeof(KeyType));
            return true;
        }

        inline bool keysAreEqual(void* a, void* b)
        {
            if(m_funckeyequalsfn != nullptr)
            {
                return m_funckeyequalsfn(a, b);
            }
            return memcmp(a, b, sizeof(KeyType)) == 0;
        }

        inline unsigned long hashKey(void* key)
        {
            if(m_funchashfn != nullptr)
            {
                return m_funchashfn(key);
            }
            return mc_util_hashdata(key, sizeof(KeyType));
        }
};
// EOF ValDict
