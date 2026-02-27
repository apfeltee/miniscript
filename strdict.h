
template<typename KeyType, typename ValType>
class StrDict
{
    #if 0
    using KeyType = char*;
    using ValType = void*;
    #endif
    public:
        enum
        {
            GDInvalidIndex = (UINT_MAX),
            DefaultInitSize = (32),
        };

    public:
        static bool initValues(StrDict* dict, unsigned int initialcapacity, mcitemcopyfn_t copyfn, mcitemdestroyfn_t dfn)
        {
            unsigned int i;
            dict->m_gdcells = nullptr;
            dict->m_gdkeys = nullptr;
            dict->m_gdvalues = nullptr;
            dict->m_gdcellindices = nullptr;
            dict->m_gdhashes = nullptr;
            dict->m_gdcount = 0;
            dict->m_gdcellcapacity = 0;
            dict->m_gdcellcapacity += initialcapacity;
            dict->m_gditemcapacity = (unsigned int)(initialcapacity * 0.7f);
            dict->m_funccopyfn = copyfn;
            dict->m_funcdestroyfn = dfn;
            dict->m_gdcells = (unsigned int*)mc_memory_malloc(dict->m_gdcellcapacity * sizeof(*dict->m_gdcells));
            dict->m_gdkeys = (KeyType*)mc_memory_malloc(dict->m_gditemcapacity * sizeof(KeyType));
            dict->m_gdvalues = (ValType*)mc_memory_malloc(dict->m_gditemcapacity * sizeof(ValType));
            dict->m_gdcellindices = (unsigned int*)mc_memory_malloc(dict->m_gditemcapacity * sizeof(*dict->m_gdcellindices));
            dict->m_gdhashes = (long unsigned int*)mc_memory_malloc(dict->m_gditemcapacity * sizeof(*dict->m_gdhashes));
            if(dict->m_gdcells == nullptr || dict->m_gdkeys == nullptr || dict->m_gdvalues == nullptr || dict->m_gdcellindices == nullptr || dict->m_gdhashes == nullptr)
            {
                goto dictallocfailed;
            }
            for(i = 0; i < dict->m_gdcellcapacity; i++)
            {
                dict->m_gdcells[i] = GDInvalidIndex;
            }
            return true;
        dictallocfailed:
            mc_memory_free(dict->m_gdcells);
            mc_memory_free(dict->m_gdkeys);
            mc_memory_free(dict->m_gdvalues);
            mc_memory_free(dict->m_gdcellindices);
            mc_memory_free(dict->m_gdhashes);
            return false;
        }

        static void deinitValues(StrDict* dict, bool freekeys)
        {
            unsigned int i;
            if(freekeys)
            {
                for(i = 0; i < dict->m_gdcount; i++)
                {
                    if(dict->m_gdkeys[i] != nullptr)
                    {
                        mc_memory_free(dict->m_gdkeys[i]);
                        dict->m_gdkeys[i] = nullptr;
                    }
                }
            }
            dict->m_gdcount = 0;
            dict->m_gditemcapacity = 0;
            dict->m_gdcellcapacity = 0;
            mc_memory_free(dict->m_gdcells);
            mc_memory_free(dict->m_gdkeys);
            mc_memory_free(dict->m_gdvalues);
            mc_memory_free(dict->m_gdcellindices);
            mc_memory_free(dict->m_gdhashes);
            dict->m_gdcells = nullptr;
            dict->m_gdkeys = nullptr;
            dict->m_gdvalues = nullptr;
            dict->m_gdcellindices = nullptr;
            dict->m_gdhashes = nullptr;
        }

        static void destroy(StrDict* dict)
        {
            if(dict != nullptr)
            {
                StrDict::deinitValues(dict, true);
                mc_memory_free(dict);
            }
        }

    public:
        unsigned int* m_gdcells;
        unsigned long* m_gdhashes;
        KeyType* m_gdkeys;
        ValType* m_gdvalues;
        unsigned int* m_gdcellindices;
        unsigned int m_gdcount;
        unsigned int m_gditemcapacity;
        unsigned int m_gdcellcapacity;
        mcitemcopyfn_t m_funccopyfn;
        mcitemdestroyfn_t m_funcdestroyfn;

    public:
        StrDict(): StrDict(nullptr, nullptr)
        {
        }

        StrDict(size_t cap, mcitemcopyfn_t copyfn, mcitemdestroyfn_t dfn)
        {
            bool ok;
            (void)ok;
            ok = StrDict::initValues(this, cap, copyfn, dfn);
            MC_ASSERT(ok);
        }

        StrDict(mcitemcopyfn_t copyfn, mcitemdestroyfn_t dfn)
        {
            bool ok;
            (void)ok;
            ok = StrDict::initValues(this, DefaultInitSize, copyfn, dfn);
            MC_ASSERT(ok);
        }


        static void destroyItemsAndDictIntern(StrDict* dict)
        {
            unsigned int i;
            if(dict != nullptr)
            {    
                if(dict->m_funcdestroyfn != nullptr)
                {
                    for(i = 0; i < dict->m_gdcount; i++)
                    {
                        dict->m_funcdestroyfn(dict->m_gdvalues[i]);
                    }
                }
                StrDict::destroy(dict);
            }
        }

        void destroyItemsAndDict()
        {
            return destroyItemsAndDictIntern(this);
        }

        bool growAndRehash()
        {
            bool ok;
            unsigned int i;

            size_t ncap;
            (void)ok;
            ncap = MC_UTIL_INCCAPACITY(m_gdcellcapacity);
            StrDict newdict(ncap, m_funccopyfn, m_funcdestroyfn);
            for(i = 0; i < m_gdcount; i++)
            {
                auto key = m_gdkeys[i];
                auto value = m_gdvalues[i];
                ok = newdict.setActual(key, key, value);
            }
            StrDict::deinitValues(this, false);
            *this = newdict;
            return true;
        }

        unsigned int getCellIndex(const void* key, unsigned long hash, bool* outfound)
        {
            unsigned int i;
            unsigned int ix;
            unsigned int cell;
            unsigned int cellix;
            unsigned long hashtocheck;
            const void* keytocheck;
            *outfound = false;
            cellix = (unsigned int)hash & (m_gdcellcapacity - 1);
            for(i = 0; i < m_gdcellcapacity; i++)
            {
                ix = (cellix + i) & (m_gdcellcapacity - 1);
                cell = m_gdcells[ix];
                if(cell == GDInvalidIndex)
                {
                    return ix;
                }
                hashtocheck = m_gdhashes[cell];
                if(hash != hashtocheck)
                {
                    continue;
                }
                keytocheck = m_gdkeys[cell];
                if(strcmp((const char*)key, (const char*)keytocheck) == 0)
                {
                    *outfound = true;
                    return ix;
                }
            }
            return GDInvalidIndex;
        }

        bool setIntern(unsigned int cellix, unsigned long hash, const KeyType ckey, KeyType mkey, ValType value)
        {
            bool ok;
            bool found;
            KeyType keycopy;
            (void)ok;
            if(m_gdcount >= m_gditemcapacity)
            {
                ok = growAndRehash();
                cellix = getCellIndex(ckey, hash, &found);
            }
            if(mkey != nullptr)
            {
                m_gdkeys[m_gdcount] = mkey;
            }
            else
            {
                keycopy = mc_util_strdup(ckey);
                if(keycopy == nullptr)
                {
                    return false;
                }
                m_gdkeys[m_gdcount] = keycopy;
            }
            m_gdcells[cellix] = m_gdcount;
            m_gdvalues[m_gdcount] = value;
            m_gdcellindices[m_gdcount] = cellix;
            m_gdhashes[m_gdcount] = hash;
            m_gdcount++;
            return true;
        }

        bool setActual(KeyType ckey, KeyType mkey, ValType value)
        {
            bool found;
            unsigned int cellix;
            unsigned int itemix;
            unsigned long hash;
            hash = mc_util_hashdata(ckey, mc_util_strlen(ckey));
            found = false;
            cellix = getCellIndex(ckey, hash, &found);
            if(found)
            {
                itemix = m_gdcells[cellix];
                m_gdvalues[itemix] = value;
                return true;
            }
            return setIntern(cellix, hash, ckey, mkey, value);
        }

        bool set(KeyType key, ValType value)
        {
            return setActual(key, nullptr, value);
        }

        template<typename InKeyT>
        ValType get(InKeyT key)
        {
            bool found;
            unsigned int itemix;
            unsigned long hash;
            unsigned long cellix;
            hash = mc_util_hashdata(key, mc_util_strlen(key));
            found = false;
            cellix = getCellIndex(key, hash, &found);
            if(!found)
            {
                return nullptr;
            }
            itemix = m_gdcells[cellix];
            return m_gdvalues[itemix];
        }

        ValType getValueAt(unsigned int ix)
        {
            if(ix >= m_gdcount)
            {
                return nullptr;
            }
            return m_gdvalues[ix];
        }

        KeyType getKeyAt(unsigned int ix)
        {
            if(ix >= m_gdcount)
            {
                return nullptr;
            }
            return m_gdkeys[ix];
        }

        size_t count()
        {
            return m_gdcount;
        }

        bool removeByKey(const KeyType key)
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
            hash = mc_util_hashdata(key, mc_util_strlen(key));
            found = false;
            cell = getCellIndex(key, hash, &found);
            if(!found)
            {
                return false;
            }
            itemix = m_gdcells[cell];
            mc_memory_free(m_gdkeys[itemix]);
            lastitemix = m_gdcount - 1;
            if(itemix < lastitemix)
            {
                m_gdkeys[itemix] = m_gdkeys[lastitemix];
                m_gdvalues[itemix] = m_gdvalues[lastitemix];
                m_gdcellindices[itemix] = m_gdcellindices[lastitemix];
                m_gdhashes[itemix] = m_gdhashes[lastitemix];
                m_gdcells[m_gdcellindices[itemix]] = itemix;
            }
            m_gdcount--;
            i = cell;
            j = i;
            for(x = 0; x < (m_gdcellcapacity - 1); x++)
            {
                j = (j + 1) & (m_gdcellcapacity - 1);
                if(m_gdcells[j] == GDInvalidIndex)
                {
                    break;
                }
                k = (unsigned int)(m_gdhashes[m_gdcells[j]]) & (m_gdcellcapacity - 1);
                if((j > i && (k <= i || k > j)) || (j < i && (k <= i && k > j)))
                {
                    m_gdcellindices[m_gdcells[j]] = i;
                    m_gdcells[i] = m_gdcells[j];
                    i = j;
                }
            }
            m_gdcells[i] = GDInvalidIndex;
            return true;
        }

        StrDict* copy()
        {
            bool ok;
            size_t i;
            StrDict* dictcopy;
            (void)ok;
            if((m_funccopyfn == nullptr) || (m_funcdestroyfn == nullptr))
            {
                return nullptr;
            }
            dictcopy = Memory::make<StrDict>((mcitemcopyfn_t)m_funccopyfn, (mcitemdestroyfn_t)m_funcdestroyfn);
            for(i = 0; i < count(); i++)
            {
                auto key = getKeyAt(i);
                auto item = getValueAt(i);
                auto itemcopy = (ValType)dictcopy->m_funccopyfn(item);
                if((item != nullptr) && (itemcopy == nullptr))
                {
                    StrDict::destroyItemsAndDictIntern(dictcopy);
                    return nullptr;
                }
                ok = dictcopy->set(key, itemcopy);
            }
            return dictcopy;
        }
};
// EOF StrDict
