/*
 * VerletNNIterator.hpp
 *
 *  Created on: Aug 16, 2016
 *      Author: i-bird
 */

#ifndef OPENFPM_DATA_SRC_NN_VERLETLIST_VERLETNNITERATOR_HPP_
#define OPENFPM_DATA_SRC_NN_VERLETLIST_VERLETNNITERATOR_HPP_


#define NO_CHECK 1
#define SAFE 2

/*! \brief Iterator for the neighborhood of the cell structures
 *
 * In general you never create it directly but you get it from the CellList structures
 *
 * It iterate across all the element of the selected cell and the near cells
 *
 * \tparam dim dimensionality of the space where the cell live
 * \tparam Cell cell type on which the iterator is working
 * \tparam NNc_size neighborhood size
 * \tparam impl implementation specific options NO_CHECK do not do check on access, SAFE do check on access
 *
 */
template<unsigned int dim, typename Ver> class VerletNNIterator
{
	size_t part_id;

	// actual element id
	size_t ele_id;

	Ver & ver;

public:

	/*! \brief
	 *
	 * Cell NN iterator
	 *
	 * \param part_id Particle id
	 * \param NNc Cell neighborhood indexes (relative)
	 *
	 */
	inline VerletNNIterator(size_t part_id, Ver & ver)
	:part_id(part_id),ele_id(0),ver(ver)
	{}

	/*! \brief
	 *
	 * Check if there is the next element
	 *
	 */
	inline bool isNext()
	{
		if (ele_id < ver.getNNPart(part_id))
			return true;
		return false;
	}

	/*! \brief take the next element
	 *
	 */
	inline VerletNNIterator & operator++()
	{
		ele_id++;

		return *this;
	}

	/*! \brief Get the value of the cell
	 *
	 * \return  the next element object
	 *
	 */
	inline size_t get()
	{
		return ver.get(part_id,ele_id);
	}
};


#endif /* OPENFPM_DATA_SRC_NN_VERLETLIST_VERLETNNITERATOR_HPP_ */