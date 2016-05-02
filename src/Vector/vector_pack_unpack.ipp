/*!
 * This file contains the implemetation of packer and unpacker for vector
 * Created on: Jan 5, 2016
 *     Author: Yaroslav Zaluzhnyi
 */

//Meta-functions to check if the packing object is complex
static bool pack()
{
	return false;
}

static bool packRequest()
{
	return false;
}

static bool packMem()
{
	return false;
}


// Structures that do a nested packing, depending on the existence of 'pack()' function inside of the object

//There is no pack() inside
template<bool cond, typename T1, typename Memory1, int ... prp>
struct pack_cond
{
	void packing(ExtPreAlloc<Memory1> & mem, openfpm::vector<T1> & obj, Pack_stat & sts)
	{
		Packer<openfpm::vector<T1>, Memory1, PACKER_ARRAY_PRIMITIVE>::pack(mem,obj,sts,obj.size());	
	}

};


//There is pack() inside
template<typename T1, typename Memory1, int ... prp>
struct pack_cond<true, T1, Memory1, prp...>
{
	void packing(ExtPreAlloc<Memory1> & mem, openfpm::vector<T1> & obj, Pack_stat & sts)
	{
	   for (size_t i = 0; i < obj.size(); i++)
		   obj.get(i).template pack<prp...>(mem, sts);
	}
};

// Structures that calculate memory for an object, depending on the existence of 'packMem()' function inside of the object

//There is no packMem() inside
template<bool cond, typename T1>
struct packMem_cond
{
	size_t packMemory(T1 & obj, size_t n, size_t e)
	{
#ifdef DEBUG
		std::cout << grow_policy_double::grow(0,n) << "*" << sizeof(T) << " " << demangle(typeid(T).name()) << std::endl;
#endif
		return grow_policy_double::grow(0,n) * sizeof(T);
	}

};

//There is packMem() inside
template<typename T1>
struct packMem_cond<true, T1>
{
	size_t packMemory(T1 & obj, size_t n, size_t e)
	{
		size_t res = 0;
		size_t count = grow_policy_double::grow(0,n) - obj.size();
		for (size_t i = 0; i < n; i++) {
			res += obj.get(i).packMem(n,0);
		}
#ifdef DEBUG
		std::cout << "Inside packMem - true (map_vector)" << std::endl;
#endif
		return res+count*sizeof(T);
	}
};


//These structures do a packing of a simple (no "pack()" inside) object 

//With specified properties
template<bool sel, int ... prp>
struct pack_simple_cond
{
	static inline void pack(const openfpm::vector<T,Memory,layout,grow_p,OPENFPM_NATIVE> & obj, ExtPreAlloc<Memory> & mem, Pack_stat & sts)
	{
	#ifdef DEBUG
		if (mem.ref() == 0)
			std::cerr << "Error : " << __FILE__ << ":" << __LINE__ << " the reference counter of mem should never be zero when packing \n";
	#endif

		//Pack the size of a vector
		Packer<size_t, Memory>::pack(mem,obj.size(),sts);
		
		// Sending property object
		typedef openfpm::vector<T> vctr;
		typedef object<typename object_creator<typename vctr::value_type::type,prp...>::type> prp_object;
	
		typedef openfpm::vector<prp_object,ExtPreAlloc<Memory>,typename memory_traits_lin<prp_object>::type ,openfpm::grow_policy_identity> dtype;
#ifdef DEBUG
	std::cout << "Inside pack_simple(not 0 prop) function! (map_vector)" << std::endl;
#endif
		// Create an object over the preallocated memory (No allocation is produced)
		dtype dest;
		dest.setMemory(mem);
		dest.resize(obj.size());
	
		auto obj_it = obj.getIterator();
	
		while (obj_it.isNext())
		{
			// copy all the object in the send buffer
			typedef encapc<1,typename vctr::value_type,typename vctr::layout_type > encap_src;
			// destination object type
			typedef encapc<1,prp_object,typename dtype::layout_type > encap_dst;
	
			// Copy only the selected properties
			object_si_d<encap_src,encap_dst,OBJ_ENCAP,prp...>(obj.get(obj_it.get()),dest.get(obj_it.get()));
	
			++obj_it;
		}
	
		// Update statistic
		sts.incReq();	
	}
};

//Without specified properties
template<int ... prp>
struct pack_simple_cond<true, prp ...>
{
	static inline void pack(const openfpm::vector<T,Memory,layout,grow_p,OPENFPM_NATIVE> & obj , ExtPreAlloc<Memory> & mem, Pack_stat & sts)
	{
	#ifdef DEBUG
		if (mem.ref() == 0)
			std::cerr << "Error : " << __FILE__ << ":" << __LINE__ << " the reference counter of mem should never be zero when packing \n";
	#endif

		//Pack the size of a vector
		Packer<size_t, Memory>::pack(mem,obj.size(),sts);
		
		// Sending property object
		typedef openfpm::vector<T,ExtPreAlloc<Memory>,layout,openfpm::grow_policy_identity> dtype;
	#ifdef DEBUG
		std::cout << "Inside pack_simple(0 prop) function! (map_vector)" << std::endl;
	#endif
		// Create an object over the preallocated memory (No allocation is produced)
		dtype dest;
		dest.setMemory(mem);
		dest.resize(obj.size());
	
		auto obj_it = obj.getIterator();
	
		while (obj_it.isNext())
		{
			// Copy
			dest.get(obj_it.get()) = obj.get(obj_it.get());
	
			++obj_it;
		}
	
		// Update statistic
		sts.incReq();
	}
};

//These structures do an upacking of a simple object (no pack() inside)

//With specified properties
template<bool sel, int ... prp>
struct unpack_simple_cond
{
	static inline void unpack(openfpm::vector<T,Memory,layout,grow_p,OPENFPM_NATIVE> & obj , ExtPreAlloc<Memory> & mem, Unpack_stat & ps)
	{
#ifdef DEBUG
	std::cout << "Inside unpack_simple(not 0 prop) function! (map_vector)" << std::endl;
#endif
		//Unpack a size of a source vector
		size_t u2 = 0;
		Unpacker<size_t, Memory>::unpack(mem,u2,ps);
		
		//Resize a destination vector
		obj.resize(u2);
		
		size_t id = 0;
		
		// Sending property object
		typedef openfpm::vector<T> vctr;
		typedef object<typename object_creator<typename vctr::value_type::type,prp...>::type> prp_object;
		typedef openfpm::vector<prp_object,PtrMemory,typename memory_traits_lin<prp_object>::type,openfpm::grow_policy_identity> stype;
		stype svect;
		

		// Calculate the size to pack the object
		size_t size = obj.packMem<prp...>(obj.size(),0);
		
		// Create a Pointer object over the preallocated memory (No allocation is produced)
		PtrMemory & ptr = *(new PtrMemory(mem.getPointerOffset(ps.getOffset()),size));
		
		stype src;
		src.setMemory(ptr);
		src.resize(obj.size());
		auto obj_it = obj.getIterator();
		
		while (obj_it.isNext())
		{
			// copy all the object in the send buffer
			typedef encapc<1,typename vctr::value_type,typename vctr::layout_type > encap_dst;
			// destination object type
			typedef encapc<1,prp_object,typename stype::layout_type > encap_src;
		
			// Copy only the selected properties
			object_s_di<encap_src,encap_dst,OBJ_ENCAP,prp...>(src.get(id),obj.get(obj_it.get()));
		
			++id;
			++obj_it;
		}
		
		ps.addOffset(size);
	}
};

//Without specified properties
template<int ... prp>
struct unpack_simple_cond<true, prp ...>
{
	static inline void unpack(openfpm::vector<T,Memory,layout,grow_p,OPENFPM_NATIVE> & obj , ExtPreAlloc<Memory> & mem, Unpack_stat & ps)
	{
#ifdef DEBUG
	std::cout << "Inside unpack_simple(0 prop) function! (map_vector)" << std::endl;
#endif
		//Unpack a size of a source vector
		size_t u2 = 0;
		Unpacker<size_t, Memory>::unpack(mem,u2,ps);
		
		//Resize a destination vector
		obj.resize(u2);
		
		size_t id = 0;
		
		// Sending property object
		typedef openfpm::vector<T,PtrMemory,layout,openfpm::grow_policy_identity> stype;

		// Calculate the size to pack the object
		size_t size = obj.packMem<prp...>(obj.size(),0);
		
		// Create a Pointer object over the preallocated memory (No allocation is produced)
		PtrMemory & ptr = *(new PtrMemory(mem.getPointerOffset(ps.getOffset()),size));
		
		stype src;
		src.setMemory(ptr);
		src.resize(obj.size());
		auto obj_it = obj.getIterator();
		
		while (obj_it.isNext())
		{
			// Copy
			obj.get(obj_it.get()) = src.get(id);
		
			++id;
			++obj_it;
		}
		
		ps.addOffset(size);
	}
};


/*! \brief Insert an allocation request into the vector
 *
 * \tparam prp list of properties
 *
 * \param v vector of allocation sequence
 *
 */
template<int ... prp> inline void packRequest(std::vector<size_t> & v) const
{
	//Pushback a sizeof number of elements of the internal vectors
	v.push_back(sizeof(this->size()));
	//std::cout << demangle(typeid(this).name()) << std::endl;
	//std::cout << this->size() << std::endl;
#ifdef DEBUG
	std::cout << "Inside map_vector.hpp packRequest()" << std::endl;
#endif
	
	// If all of the aggregate properties do not have a "pack()" member	
	if (has_pack_agg<T,prp...>::result::value == false)
	{
#ifdef DEBUG
			std::cout << "All of the aggregate members are simple!(packRequest)" << std::endl;
#endif
		size_t alloc_ele = this->packMem<prp...>(this->size(),0);
		v.push_back(alloc_ele);
	}
	//If at least one property has "pack()"
	else
	{
#ifdef DEBUG
			std::cout << "Not all of the aggregate members are simple!(packRequest)" << std::endl;
#endif
		for (size_t i = 0 ; i < this->size() ; i++)
		{
			//Call a pack request
			call_aggregatePackRequest<T,Memory,layout,grow_p,prp ... >::call_packRequest(*this,v);
		}
	}
}


/*! \brief pack a vector selecting the properties to pack
 *
 * \param mem preallocated memory where to pack the vector
 * \param sts pack-stat info
 *
 */
template<int ... prp> inline void pack(ExtPreAlloc<Memory> & mem, Pack_stat & sts) const
{
	//If all of the aggregate properties are simple (don't have "pack()" member)
	if (has_pack_agg<T,prp...>::result::value == false)
	//if (has_aggregatePack<T,prp ... >::has_pack() == false)
	{
#ifdef DEBUG
		std::cout << "All of the aggregate members are simple!(pack)" << std::endl;
#endif
		//Call a packer
		pack_simple_cond<sizeof...(prp) == 0,prp...>::pack(*this,mem,sts);
	}
	//If at least one property has a "pack()" member
	else
	{
#ifdef DEBUG
		std::cout << "Not all of the aggregate members are simple!(pack)" << std::endl;
#endif
		//Pack the size of a vector
		Packer<size_t, Memory>::pack(mem,this->size(),sts);
		
		//Call a packer in nested way
		call_aggregatePack<T,Memory,layout,grow_p,prp ... >::call_pack(*this,mem,sts);
	}
}

/*! \brief unpack a vector
 *
 * \param mem preallocated memory from where to unpack the vector
 * \param ps unpack-stat info
 */
template<int ... prp> inline void unpack(ExtPreAlloc<Memory> & mem, Unpack_stat & ps)
{
	//if all of the aggregate properties are simple (don't have "pack()" member)
	if (has_pack_agg<T,prp...>::result::value == false)
	//if (has_aggregatePack<T,prp ... >::has_pack() == false)
	{
#ifdef DEBUG
		std::cout << "All of the aggregate members are simple!(unpack)" << std::endl;
#endif
		//Call an unpacker
		unpack_simple_cond<sizeof...(prp) == 0,prp...>::unpack(*this,mem,ps);
	}
	//If at least one is not simple (has a "pack()" member)
	else
	{
#ifdef DEBUG
		std::cout << "Not all of the aggregate members are simple!(unpack)" << std::endl;
#endif
		//Unpack a size of a source vector
		size_t u2 = 0;
		Unpacker<size_t, Memory>::unpack(mem,u2,ps);
		
		//Resize a destination vector
		this->resize(u2);
		
		//Call an unpacker in nested way
		call_aggregateUnpack<T,Memory,layout,grow_p,prp ... >::call_unpack(*this,mem,ps);
	}
}

