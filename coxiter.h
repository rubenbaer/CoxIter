/*
Copyright (C) 2013, 2014, 2015, 2016
Rafael Guglielmetti, rafael.guglielmetti@unifr.ch
*/

/*
This file is part of CoxIter.

CoxIter is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

CoxIter is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with CoxIter. If not, see <http://www.gnu.org/licenses/>.
*/

/*!
 * \file coxiter.h
 * \author Rafael Guglielmetti
 * 
 * \class CoxIter
 * \brief Main class for the work
 * 
 * \remark This must be compiled with C++11 and a few other stuff (see documentation)
*/

#ifndef __COXITER_H__
#define __COXITER_H__ 1

#include "graphs.list.h"
#include "graphs.list.iterator.h"
#include "graphs.product.h"
#include "graphs.product.set.h"
#ifndef _COMPILE_WITHOUT_REGEXP_
#include "lib/regexp.h"
#endif
#include "lib/math_tools.h"
#include "lib/polynomials.h"
#include "lib/numbers/mpz_rational.h"

#include <algorithm>
#include <iterator>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <map>
#include <cmath>
#include <unordered_set>

#ifdef _USE_LOCAL_GMP_
#include "gmpxx.h"
#else
#include <gmpxx.h>
#endif

#ifdef _OPENMP
#include <omp.h>
#else
inline unsigned int omp_get_thread_num() { return 0; }
inline unsigned int omp_get_max_threads() { return 1; }
#endif

using namespace std;
using namespace MathTools;

class CoxIter
{
	private:
		string strError; ///< Error code
		bool bDebug; ///< If true, prints additionnal information
		
		bool bUseOpenMP; ///< Use OpenMP
		
		// -----------------------------------------------------------
		// I/O
		bool bWriteInfo; ///< If we want to write informations (false if CoxIter is used "as a plugin")
		
		// -----------------------------------------------------------
		// Redirection cout to a file
		bool bCoutFile; ///< True if we want to redirect cout to a file
		ofstream *outCout; ///< Flow to the file
		streambuf *sBufOld; ///< To reset the cout, at the end
		
		string strOuputMathematicalFormat; ///< Format for mathematical output (generic, mathematica)
		
		// -----------------------------------------------------------
		// Graph
		unsigned int iVerticesCount; ///< Number of vertices
		unsigned int iMaximalSubgraphRank; ///< Maximal rank of a subgraph
		
		unsigned int iDimension; ///< Dimension (or 0)
		unsigned int iSphericalMaxRankFound; ///< Maximal rank for a spherical graph
		unsigned int iEuclideanMaxRankFound; ///< Maximal rank for an euclidean graph
		bool bDimension_guessed; ///< If the dimension was not specified but guessed
		
		bool bGraphExplored; ///< True if we looked for connected subgraphs (affine and spherical)
		bool bGraphsProductsComputed; ///< True if we computed the graphs products
		bool bGrowthSeriesComputed; ///< True if we computed the growth series
		
		bool bHasDottedLine; ///< True if the graph has a dotted line
		int iHasDottedLineWithoutWeight; ///< If the graph dotted lines without weight (-1: maybe, 0: no, 1: yes)
		bool bHasBoldLine; ///< True if the graph has a bold line
		bool bCheckCocompactness; ///< True if we want to check the cocompacity
		bool bCheckCofiniteness; ///< True if we want to check the finite covolume condition
		int iIsCocompact; ///< 1 If cocompact, 0 if not, -1 if don't know, -2 if not tested
		int iIsArithmetic; ///< 1 If arithmetic, 0 if non-arithmetic, -1 if don't know
		int iIsFiniteCovolume; ///< 1 If finite covolume, 0 if not, -1 if don't know (or cannot know), -2 if not tested
		
		vector< string > strVerticesRemove; ///< Vertices to be removed
		vector< string > strVertices; ///< Vertices to be taken
		
		map< string, unsigned int > map_vertices_labelToIndex; ///< For the correspondance: label <-> indexes of vertices
		vector< string > map_vertices_indexToLabel; ///< For the correspondance: label <-> indexes of vertices
                
		vector< vector <unsigned int > > iCoxeterMatrix; ///< Coxeter matrix
		map< unsigned int, string > strWeights; ///< Weights of the dotted lines (via linearization)
		vector< vector<bool> > bEdgesVisited; ///< For the DFS: traversed edges
		vector< bool > bVerticesVisited; ///<  For the DFS: traversed vertices
		vector<short unsigned int> iPath; ///< chemin en cours (pour le DFS)
		
		string strGramMatrixField; ///< Field generated by the entries of the Gram matrix
		bool bGramMatrixField; ///< True if the field was determined
		
		GraphsList *graphsList_spherical; ///< Pointer to the list of spherical graphs
		GraphsList *graphsList_euclidean; ///< Pointer to the list of euclidean graphs
		
		// Computations relative to the infinite sequence
		unsigned int iISt0; ///< First reflecting hyperplane
		unsigned int iISs0; /// (ultra)parallel hyperplane, will be conjugate
		vector< unsigned int > iISFVectorsUnits; ///< Components of the f-vector
		vector< unsigned int > iISFVectorsPowers; ///< Components of the f-vector
		
		// -----------------------------------------------------------
		// Graphs products
		/*! \var graphsProducts(vector< vector< GraphsProductSet > >)
		 * [0] Spherical products of codimension 1
		 * [1] Spherical products of codimension 0
		 * [2] Euclidean products of codimension 0
		 * 
		 * OR
		 * 
		 * [i] => Euclidean products of rank i (for bCanBeFiniteCovolume_complete)
		 */
		vector< vector< GraphsProductSet > > graphsProducts;
		
		/*! \var graphsProducts_bCanBeFiniteCovolume(vector< vector< GraphsProductSet > >)
		 * Used in bCanBeFiniteCovolume and bCanBeFiniteCovolume_complete
		 * [0] Euclidean products of codimension 0
		 * 
		 * OR
		 * 
		 * [i] => Euclidean products of rank i
		 */
		vector< vector< GraphsProductSet > > graphsProducts_bCanBeFiniteCovolume;
	
		/*! 	\var graphsProductsCount_spherical
		 * 	\var graphsProductsCount_euclidean
		 * 	\brief Count graphs products (with their multiplicities)
		 * 
		 * 	External vectors: products of graphs by their number of total vertices<br />
		 * 	map< vector< vector<short unsigned int> >, unsigned int><br>
		 * 		The key is "vector< vector<short unsigned int> >": For each type of graph and each rank, how many times it occurs in the product<br />
		 * 			For example, the vector [ 0 => [2, 3], 3 => [ 1 ] ] corresponds to: A1 x A1 x A2 x A2 X A2 x D3<br>
		 * 		la valeur est le nombre de fois que le produit apparait<br>
		 */
		vector< map<vector< vector<short unsigned int> >, unsigned int> > graphsProductsCount_spherical;
		vector< map<vector< vector<short unsigned int> >, unsigned int> > graphsProductsCount_euclidean;
		
		vector< mpz_class > iFactorials, iPowersOf2; ///< Some factorials and powers of two
	
		// ------------------------------------------------------------
		// Results
		MPZ_rational brEulerCaracteristic; ///< Euler characteristic
		string strEulerCharacteristic_computations; ///< Euler characteristic (without the computations done)
		
		int iFVectorAlternateSum; ///< Alternating sum of the components of the f-vector
		vector<unsigned int> iFVector; ///< F-vector
		unsigned int iVerticesAtInfinityCount; ///< Number of vertices at infinity
		
		vector< mpz_class > growthSeries_iPolynomialDenominator; ///< (i-1)th term contains the coefficient of x^i
		vector<unsigned int> growthSeries_iCyclotomicNumerator; ///< Contains a list oif cyclotomic polynomials
		bool growthSeries_bFractionReduced; ///< True if the fraction has been reduced (it is always the case when the cyclotomic terms are <= 60, which is... always (except if we find an hyperbolic group in H^31))
		
		string growthSeries_raw; ///< Row series, not simplified
		
	public:
		/*! \fn CoxIter()
		 * 	\brief Default constructor. Initialize the default values.
		 */
		CoxIter();
		
		/*! \fn CoxIter(const vector< vector<unsigned int> >& iMatrix, const unsigned int& iDimension)
		 * 	\brief Constructor
		 * 	\param iMatrix(const vector< vector<unsigned int> >&) Coxeter matrix
		 * 	\param iDimension(const unsigned int &) Dimension
		 * 
		 * 	CoxIter does not verification on iMatrix. Especially, it is assumed that iMatrix is symmetric
		*/
		CoxIter(const vector< vector<unsigned int> >& iMatrix, const unsigned int& iDimension);
		
		~CoxIter();
		
		/*!	\fn bRunAllComputations
		 * 	\brief Do all the computations
		 * 
		 * 	Call the followings functions:<br />
		 * 		readGraph()<br />
				exploreGraph()<br />
				computeGraphsProducts()<br />
				euler()<br />
				isFiniteCovolume()<br />
				iIsGraphCocompact()<br />
				
			\return True if success
		 */
		bool bRunAllComputations();
		
		/*!	\fn printCoxeterMatrix
		 * 	\brief Print Coxeter matrix
		 */
		void printCoxeterMatrix();
		
		/*!	\fn printCoxeterGraph
		 * 	\brief Print Coxeter graph
		 */
		void printCoxeterGraph();
		
		/*!	\fn printGramMatrix
		 * 	\brief Print the Gram matrix
		 */
		void printGramMatrix();
		
		/*!	\fn printGramMatrix_GAP
		 * 	\brief Print the Gram matrix (format: GAP)
		 */
		void printGramMatrix_GAP();
		
		/*!	\fn printGramMatrix_Mathematica
		 * 	\brief Print the Gram matrix (format: Mathematica)
		 */
		void printGramMatrix_Mathematica();
		
		/*!	\fn printGramMatrix_PARI
		 * 	\brief Print the Gram matrix (format: PARI)
		 */
		void printGramMatrix_PARI();
		
		/*!	\fn printGramMatrix_LaTeX
		 * 	\brief Print the Gram matrix (format: LaTeX)
		 */
		void printGramMatrix_LaTeX();
		
		/*!	\fn printEdgesVisitedMatrix
		 * 	\brief Display the visited edges
		 */
		void printEdgesVisitedMatrix();

		/*! \fn bReadGraphFromFile
		 * 	\brief Read the graph from a file
		 * 
		 * 	\param strInputFilename(const string&) Path to the file
		 * 	\return True if success
		 */
		#ifndef _COMPILE_WITHOUT_REGEXP_
		bool bReadGraphFromFile(const string& strInputFilename);
		#endif
		
		/*!	\fn bWriteGraphToDraw
		 * 	\brief Write the graph in a file for GraphViz
		 * 
		 * 	The graph is written in szOutputGraphFilename + ".graphviz"
		 * 	\param strOutFilenameBasis(const string&) Filename
		 *	\return True if OK, false otherwise
		 */
		bool bWriteGraphToDraw(const string& strOutFilenameBasis);
		
		/*! 	\fn bWriteGraph
		 *	\brief Write the graph in a file (so that it can be read by CoxIter)
		 * 
		 *	\param strFilename(const string &)
		 * 	\return True if success, false otherwise
		 */
		bool bWriteGraph(const string& strFilename);
		
		/*!	\fn parseGraph
		 * 	\brief Read and parse graph from stream
		 * 
		 * 	\param streamIn(const ifstream&) Stream to the content (file or std::cin)
		 * 	\return True if success
		 */
		#ifndef _COMPILE_WITHOUT_REGEXP_
		bool parseGraph(istream& streamIn);
		#endif
		
		/*!
		 * 	\fn exploreGraph
		 * 	\brief Explore the graph (via iCoxeterMatrix) to gind subgraphs
		 * 
		 * 	First, we find all the chains startings from every vertex. Then, we wxpand the chains to spherical and euclidean graphs
		 */
		void exploreGraph();
		
		/*!
		 * 	\fn IS_computations
		 * 	\brief Do some computations related to the infinite sequence
		 * 	Remark: It is suppose that both t0, s0 are admissible vertices whose corresponding hyperplanes are (ultra)parallel
		 * 
		 * \param t0(const string&) t0 Reflecting hyperplane
		 * \param t0(const string&) s0 Other hyperplane
		 */
		void IS_computations(const string& t0, const string& s0);

		/*!	\fn bEulerCharacteristicFVector
		 * 	\brief Conmpute the euler characteristic and f-vector
		 * 	\return True if success
		 */
		bool bEulerCharacteristicFVector();
		
		/*!	\fn growthSeries
		 * 	Compute the growth series
		 * 
		 * 	\remark The only purpose of this function is to call growthSeries_parallel or growthSeries_sequential
		 */
		void growthSeries();

		/*!	\fn iIsGraphCocompact
		 * 	\brief Check whether the graph is cocompact or not
		 * 	Remark: If the programm was not called with the -compacity flag, the function does nothing
		 * 	\return Value of iIsCocompact
		 */
		int iIsGraphCocompact();
		
		/*!	\fn isFiniteCovolume
		 * 	\brief Check whether the graph is of finite covolume or not
		 * 	Remark: If the programm was not called with the -fv flag, the function does nothing
		 * 	\return Value of iIsFiniteCovolume
		 */
		int isFiniteCovolume();
		
		/*!	\fn bCanBeFiniteCovolume
		 * 	\brief Check whether the group can be of finite covolume or no
		 * 	Remark: If true, it does not mean that the group is of finite covolume. The function only provides a faster test if we think that the group is of infinite covolume.
		 * 	We also suppose that this function is called alone most of the time (i.e. that we won't call bCheckFiniteCovolume after that, most of the time).
		 * 	\return True is the group can be of finite covolume, false if the group is of infinite covolume
		 */
		bool bCanBeFiniteCovolume();
		
		/*!	\fn bCanBeFiniteCovolume_complete
		 * 	\brief Check whether the group can be of finite covolume or no
		 * 	\return The list of affine graphs which cannot be extended to an affine graph of rank n-1. If the list is empty, then it is possible that the group has finite covolume.
		 */
		vector< vector<short unsigned int> > bCanBeFiniteCovolume_complete();

		/*!
		 * \fn computeGraphsProducts
		 * \brief Compute the possible products of the irreducible graphs
		 */
		void computeGraphsProducts();
		
		/*!
		 * \fn printGrowthSeries
		 * \brief Display the growth series
		 */
		void printGrowthSeries();
		
		/*! \fn printEuclideanGraphsProducts
		 * 	\brief Display the euclidean graph products found
		 * 
		 * 	\param graphsProductsCount(vector< map<vector< vector<short unsigned int> >, unsigned int> >*) Pointer to the vector contaitning the results
		 */
		void printEuclideanGraphsProducts(vector< map<vector< vector<short unsigned int> >, unsigned int> >* graphsProductsCount);
		
		/*! 	\fn bIsVertexValid
		 * 	\brief Test if a vertex exists in the graph
		 * 	\param strVertexLabel(const string&) Label of the vertex
		 * 	\return True if the vertex exists, false otherwise
		 */
		bool bIsVertexValid(const string& strVertexLabel) const;
		
		/*! 	\fn get_iVertexIndex
		 * 	\brief Get the index of a vertex
		 * 	\param strVertexLabel(const string&) Label of the vertex
		 * 	\return Index of the vertex (throw an exception if the vertex does not exist)
		 */
		unsigned int get_iVertexIndex(const string& strVertexLabel) const;
		
		/*! 	\fn get_strVertexLabel
		 * 	\brief Get the label of a vertex
		 * 	\param iVertex(const unsigned int&) Index of the vertex
		 * 	\return Label of the vertex (throw an exception if the vertex does not exist)
		 */
		string get_strVertexLabel(const unsigned int& iVertex) const;
		
		/*! \fn get_strError
		 * \brief Retourne le code d'erreur
		 * 
		 * \return Code d'erreur (string)
		 */
		string get_strError() const;
		
		/*! \fn get_brEulerCaracteristic
		 * \brief return brEulerCaracteristic
		 * 
		 * \return brEulerCaracteristic (MPZ_rational)
		 */
		MPZ_rational get_brEulerCaracteristic() const;
		
		/*! \fn get_strEulerCaracteristic
		 * \brief return brEulerCaracteristic
		 * 
		 * \return Euler characteristic (string)
		 */
		string get_strEulerCaracteristic() const;
		
		/*! \fn get_strEulerCharacteristic_computations
		 * \brief Return the computations needed to determine Euler's characteristic
		 * 
		 * \return strEulerCharacteristic_computations (string)
		 */
		string get_strEulerCharacteristic_computations() const;
		
		/*! \fn get_iFVectorAlternateSum
		 * 	\brief Return the alternating sum of the componenents of the f-vector
		 * 	\return Alternating sum of the componenents of the f-vector (int)
		 */
		int get_iFVectorAlternateSum() const;
		
		/*! \fn get_iFVector
		 * 	\brief Return the f-vector
		 * 	\return f-vector
		 */
		vector< unsigned int> get_iFVector() const;
		
		/*! \fn get_iISFVectorsUnits
		 * 	\brief Return the units of the f-vector after n-doubling
		 * 	\return Units
		 */
		vector< unsigned int > get_iISFVectorsUnits() const;
		
		/*! \fn get_iISFVectorsUnits
		 * 	\brief Return the powers of 2^{n-1} of the f-vector after n-doubling
		 * 	\return Units
		 */
		vector< unsigned int > get_iISFVectorsPowers() const;
		
		/*! \fn get_iVerticesAtInfinityCount
		 * 	\brief Return the number of vertices at infinity
		 * 	\return Return the number of vertices at infinity
		 */
		unsigned int get_iVerticesAtInfinityCount() const;
		
		/*! \fn get_iIrreducibleSphericalGraphsCount
		 * 	\brief Return the number of irreducible spherical graphs
		 * 	\return Return the number of irreducible spherical graphs
		 */
		unsigned int get_iIrreducibleSphericalGraphsCount() const;
		
		/*!
		 * 	\fn get_bWriteInfo
		 * 	\brief Return bWriteInfo
		 * 	\return bWriteInfo
		 */
		bool get_bWriteInfo() const;
		
		/*!
		 * 	\fn get_bDebug
		 * 	\brief Return get_bDebug
		 * 	\return get_bDebug
		 */
		bool get_bDebug() const;
		
		/*!
		 * 	\fn get_iDimension
		 * 	\brief Return the dimension
		 * 	\return Dimension (0 if not specified/guessed)
		 */
		unsigned int get_iDimension() const;
		
		/*!
		 * 	\fn get_bDimensionGuessed
		 * 	\brief Return true if the dimension was guessed
		 * 	\return True if the dimension was guessed
		 */
		bool get_bDimensionGuessed() const;
		
		/*!
		 * 	\fn get_iIsCocompact
		 * 	\brief Return the value of iIsCompact
		 * 	\return 1 if cocompact, 0 if not, -1 if not tested
		 */
		int get_iIsCocompact();
		
		/*!
		 * 	\fn get_iIsFiniteCovolume
		 * 	\brief Return the value of iIsFiniteCovolume
		 * 	\return 1 if finite covolume, 0 if not, -1 if not tested
		 */
		int get_iIsFiniteCovolume();
		
		/*!
		 * 	\fn get_iIsArithmetic
		 * 	\brief Arithmetic?
		 * 	\return 1 if arithmetic, 0 if not, -1 if not known
		 */
		int get_iIsArithmetic() const;
		
		/*!
		 * 	\fn get_iCoxeterMatrix
		 * 	\brief Return the Coxeter matrix
		 * 	\return Coxeter matrix
		 */
		vector< vector<unsigned int> > get_iCoxeterMatrix() const;
		
		/*!
		 * 	\fn get_iCoxeterMatrixEntry
		 * 	\brief Return the one entry of the Coxeter matrix
		 * 	\return Entry
		 */
		unsigned int get_iCoxeterMatrixEntry(const unsigned int& i, const unsigned int& j) const;
		
		/*!
		 * 	\fn get_iCoxeterMatrix
		 * 	\brief Return the weights of the dotted lines
		 * 	\return Weights of the dotted lines
		 */
		map< unsigned int, string > get_strWeights() const;
		
		/*!
		 * 	\fn get_strCoxeterMatrix
		 * 	\brief Return the Coxeter matrix
		 * 	\return Coxeter matrix (string)
		 */
		string get_strCoxeterMatrix() const;
		
		/*!
		 * 	\fn get_array_str_2_GramMatrix
		 * 	\brief Return the entries of 2*G (string)
		 * 	\return Entries of 2*G
		 */
		vector< vector< string > > get_array_str_2_GramMatrix() const;
		
		/*!	\fn get_strGramMatrix
		 * 	\brief Returns the Gram matrix
		 * 	\return Gram matrix (string)
		 */
		string get_strGramMatrix() const;
		
		/*!	\fn get_strCoxeterGraph
		 * 	\brief Returns the Coxeter graph
		 * 	\return Gram graph (string)
		 */
		string get_strCoxeterGraph() const;
		
		/*!	\fn get_strGramMatrix_GAP
		 * 	\brief Returns the Gram matrix (format GAP)
		 * 	\return Gram matrix (string)
		 */
		string get_strGramMatrix_GAP() const;
		
		/*!	\fn get_strGramMatrix_LaTeX
		 * 	\brief Returns the Gram matrix (format LaTeX)
		 * 	\return Gram matrix (string)
		 */
		string get_strGramMatrix_LaTeX() const;
		
		/*!	\fn get_strGramMatrix_Mathematica
		 * 	\brief Returns the Gram matrix (format Mathematica)
		 * 	\return Gram matrix (string)
		 */
		string get_strGramMatrix_Mathematica() const;
		
		/*!	\fn get_strGramMatrix_PARI
		 * 	\brief Returns the Gram matrix (format PARI)
		 * 	\return Gram matrix (string)
		 */
		string get_strGramMatrix_PARI() const;
		
		/*!	\fn get_strGramMatrixField
		 * 	\brief Field generated by the entries of the Gram matrix (string)
		 * 	\return Field generated by the entries of the Gram matrix (string)
		 */
		string get_strGramMatrixField() const;
		
		/*!	\fn get_iVerticesCount
		 * 	\brief Retourne le nombre de sommets du graphe
		 * 	\return Retourne le nombre de sommets du graphe (int)
		 */
		unsigned int get_iVerticesCount() const;
		
		/*!	\fn get_bHasDottedLine
		 * 	\brief Does the graph have at least one dotted edge?
		 * 	\return Yes if the graph has at least one dotted edge
		 */
		bool get_bHasDottedLine() const;
		
		/*!	\fn get_iHasDottedLineWithoutWeight
		 * 	\brief Does the graph have dotted edges without weights?
		 * 	\return -1: maybe, 0: no, 1: yes
		 */
		int get_iHasDottedLineWithoutWeight() const;
		
		/*!	\fn get_str_map_vertices_indexToLabel
		 * 	\brief Return the label of the vertices
		 * 	\return The labels of the vertices
		 */
		vector< string > get_str_map_vertices_indexToLabel() const;
		
		/*!
		 * 	\fn set_iIsArithmetic
		 * 	\brief Update the member iIsArithmetic
		 * 	
		 * 	This is used by the Arithmeticity class
		 */
		void set_iIsArithmetic(const unsigned int &iArithmetic);
		
		void set_bCheckCocompactness(const bool& bValue);
		void set_bCheckCofiniteness(const bool& bValue);
		void set_bDebug(const bool& bValue);
		void set_bUseOpenMP(const bool& bValue);
		void set_strOutputFilename(const string& strValue);
		void set_sdtoutToFile(const string& strFilename);
		void set_strVerticesToRemove(const vector< string >& strVerticesRemove_);
		void set_strVerticesToConsider(const vector< string >& strVerticesToConsider);
		
		/*!
		 * 	\fn set_bWriteInfo
		 * 	\brief Set bWriteInfo
		 * 	\param bNewValue(const bool&) The new value
		 * 	\return void
		 */
		void set_bWriteInfo(const bool& bNewValue);
		
		/*!
		 * 	\fn set_iDimension
		 * 	\brief Update the member iDimension
		 */
		void set_iDimension(const unsigned int& iDimension_); 
		
		GraphsList* get_gl_graphsList_spherical() const;
		
		GraphsList* get_gl_graphsList_euclidean() const;
		
		bool get_b_hasSphericalGraphsOfRank(const unsigned int& iRank) const;
		
		bool get_b_hasEuclideanGraphsOfRank(const unsigned int& iRank) const;
		
		/*!
		 * 	\fn get_iGrowthSeries
		 * 	\brief Return the growth series of the group
		 * 	
		 * 	\param iCyclotomicNumerator(vector<unsigned int>&) Numerator (cyclotomic factors)
		 * 	\param iPolynomialDenominator(vector< mpz_class >&) Denominator
		 * 	\param bReduced(bool&) True if the fraction is reduced
		 */
		void get_iGrowthSeries(vector<unsigned int>& iCyclotomicNumerator, vector< mpz_class >& iPolynomialDenominator, bool& bReduced);
		
		/*!
		 * 	\fn get_bGrowthSeriesReduced
		 * 	\brief Return true if the fraction is reduced
		 * 	
		 * 	\return True if the fraction is reduced
		 */
		bool get_bGrowthSeriesReduced();
		
		vector< mpz_class > get_iGrowthSeries_denominator();
		
		string get_strGrowthSeries();
		string get_strGrowthSeries_raw();
		
		/*!
		 * 	\fn get_ptr_graphsProducts
		 * 	\brief Return the list of graphs products
		 * 	Remark: there is absolutely no verification
		 */
		const vector< vector< GraphsProductSet > >* get_ptr_graphsProducts() const;
		  
		/*!
		 * 	\fn set_iCoxeterMatrix
		 * 	\brief Set the Coxeter matrix
		 * 	\param iMat(const vector< vector<short unsigned int> >&) The matrix
		 */
		void set_iCoxeterMatrix(const vector< vector<unsigned int> >& iMat);
		
		void set_strOuputMathematicalFormat(const string& strO);
		
		/*!
		 * 	\fn map_vertices_labels_removeReference
		 * 	\brief Remove the references to a vertex (for the label)
		 * 	\param iIndex(const unsigned int&) iIndex of the vertex
		 */
		void map_vertices_labels_removeReference(const unsigned int& iIndex);
		
		/*!
		 * 	\fn map_vertices_labels_addReference
		 * 	\brief Add a references for a new vertex
		 * 	\param strLabel(const string&) Label of the vertec
		 */
		void map_vertices_labels_addReference(const string& strLabel);
		
		/*!
		 * 	\fn map_vertices_labels_create
		 * 	\brief Create labels if there is none (int --> string)
		 */
		void map_vertices_labels_create();
		
		/*!
		 * 	\fn map_vertices_labels_reinitialize
		 * 	\brief Create labels if there with int
		 */
		void map_vertices_labels_reinitialize();
                
	private:
		CoxIter(const CoxIter&); ///< We do not want to do this
		
		/*! 	\fn initializations
		 * 	\brief Une fois le nombre de sommets du graphe connu (via inputRead()), fait divers initialisations de variables
		 */
		void initializations();
		
		/*! \fn DFS
		 * \brief Look for all the An starting from a given vertex
		 * 
		 * Thie function calls addGraphsFromPath() for each maximal An found
		 * 
		 * \param iRoot Starting point
		 * \param iFrom Previous vertex (or iRoot if first call)
		 */
		void DFS(unsigned int iRoot, unsigned int iFrom);
		
		/*! 	\fn printPath
		 * 	\brief Print the iPath vector
		 */
		void printPath();
		
		/*!	\fn addGraphsFromPath
		 * 	\brief Find the different type of graphs (An, Bn, Dn, En, Hn, F4) from any An
		 * 
		 * 	Based on the content of iPath
		 */
		void addGraphsFromPath();
		
		/*!
		 * \fn AnToEn_AnToTEn(const vector<short unsigned int>& iPathTemp, const vector< bool >& bVerticesLinkable)
		 * \brief Essaie de construire des En depuis un An
		 * 
		 * \param iPathTemp(vector<short unsigned int>&) Chemin actuel composant le An
		 * \param bVerticesLinkable(const vector< bool >&) Ce qui est liable ou non au graphe
		 */
		void AnToEn_AnToTEn(const vector<short unsigned int>& iPathTemp, const vector< bool >& bVerticesLinkable);
		
		/*!
		 * \fn AnToEn_AnToTEn(const vector<short unsigned int>& iPathTemp, const vector< bool >& bVerticesLinkable, const bool& bSpherical, const short unsigned int& iStart)
		 * \brief Try to foind an En from an An
		 * 
		 * \param iPathTemp (const vector<unsigned int>&) Vertices of the An
		 * \param bVerticesLinkable(const vector<bool> &) Linkable vertices 	
		 * \param bSpherical(const bool&) True if spherical, false if euclidean
		 * \param iStart(const unsigned int&) Starting point
		 */
		void AnToEn_AnToTEn(const vector<short unsigned int>& iPathTemp, const vector< bool >& bVerticesLinkable, const bool& bSpherical, const short unsigned int& iStart);
		
		/*!
		 * \fn B3ToF4_B4ToTF4
		 * \brief Essaie de construire un F4 depuis un B3
		 * 
		 * \param bVerticesBeginLinkable(const vector<bool> &) What's linkable to the B3
		 * \param iPathTemp (vector<unsigned int>) Vertices of the B3
		 * \param iVEnd Index of the vertex connected by a 4
		 */
		void B3ToF4_B4ToTF4(const vector<bool> &bVerticesBeginLinkable, vector<short unsigned int> iPathTemp, const short unsigned int &iVEnd);
		
		/*!	\fn computeGraphsProducts(GraphsListIterator grIt, vector< map<vector< vector<short unsigned int> >, unsigned int> >* graphsProductsCount, const bool& bSpherical, GraphsProduct& gp, vector< bool >& bGPVerticesNonLinkable)
		 * 	\brief Try to find products of connected graphs
		 * 
		 * 	\param grIt(GraphsListIterator): Iterator on the list
		 * 	\param graphsProductsCount(vector< map<vector< vector<short unsigned int> >, unsigned int> >*) Point to the list of graphs
		 * 	\param bSpherical(const bool&): True if spherical, false if euclidean
		 * 	\param gp(GraphsProduct&) To store the product (for the cocompacity and finite covolume tests)
		 * 	\param bGPVerticesNonLinkable(vector< bool >&) Vertices which cannot be linked to the current product
		 */
		void computeGraphsProducts(GraphsListIterator grIt, vector< map<vector< vector<short unsigned int> >, unsigned int> >* graphsProductsCount, const bool& bSpherical, GraphsProduct& gp, vector< bool >& bGPVerticesNonLinkable);
		
		/*!	\fn computeGraphsProducts_IS(GraphsListIterator grIt, vector< map<vector< vector<short unsigned int> >, unsigned int> >* graphsProductsCount, const bool& bSpherical, GraphsProduct& gp, vector< bool >& bGPVerticesNonLinkable)
		 * 	\brief Compute the FVector for the infinite sequence
		 * 
		 * 	\param grIt(GraphsListIterator): Iterator on the list
		 * 	\param bSpherical(const bool&): True if spherical, false if euclidean
		 * 	\param gp(GraphsProduct&) To store the product (for the cocompacity and finite covolume tests)
		 * 	\param bGPVerticesNonLinkable(vector< bool >&) Vertices which cannot be linked to the current product
		 */
		void computeGraphsProducts_IS(GraphsListIterator grIt, const bool& bSpherical, GraphsProduct& gp, vector< bool >& bGPVerticesNonLinkable);
		
		void bCanBeFiniteCovolume_computeGraphsProducts(GraphsListIterator grIt, GraphsProduct& gp, vector< bool >& bGPVerticesNonLinkable);
		void bCanBeFiniteCovolume_complete_computeGraphsProducts(GraphsListIterator grIt, GraphsProduct& gp, vector< bool >& bGPVerticesNonLinkable);
		
		/*!	\fn i_orderFiniteSubgraph
		 * 	\brief Order of a connected spherical graph
		 * 
		 * 	\param iType Type du graphe (0 = An, 1=Bn, ...)
		 * 	\param iDataSupp Valeur du n pour presque tous les graphes, poids pour un G_2^n
		 * 
		 * 	\return Ordre (unsigned long int)
		 */
		mpz_class i_orderFiniteSubgraph(const unsigned int &iType, const unsigned int &iDataSupp);
		
		/*!	\fn b_isGraph_cocompact_finiteVolume_parallel
		 * 	\brief Check whether the graph is cocompact or not or has finite covolume or not
		 * 	Called by: iIsGraphCocompact and iIsFiniteCovolume
		 * 	\param iIndex(unsigned int): 1 if test for compacity, 2 if test for the finite covolume
		 * 	\return True or false
		 */
		bool b_isGraph_cocompact_finiteVolume_parallel(unsigned int iIndex);
		
		/*!	\fn b_isGraph_cocompact_finiteVolume_sequential
		 * 	\brief Check whether the graph is cocompact or not or has finite covolume or not
		 * 	Called by: iIsGraphCocompact and iIsFiniteCovolume
		 * 	\param iIndex(unsigned int): 1 if test for compacity, 2 if test for the finite covolume
		 * 	\return True or false
		 */
		bool b_isGraph_cocompact_finiteVolume_sequential(unsigned int iIndex);
		
		/*!	\fn growthSeries_symbolExponentFromProduct(const vector< vector<short unsigned int> >& iProduct, vector<unsigned int>& iSymbol, unsigned int& iExponent) const
		 * 	From a product of graphs, compute the corresponding symbol [n1, n2, ..., nk] together with the exponent.
		 * 	
		 * 	\param iProduct(const vector< vector<short unsigned int> >&) The product of graphs
		 * 	\param iSymbol(vector<short unsigned int>&) [i] = j means [i, ..., i] j times (parameter by reference)
		 * 	\param iExponent(unsigned int&) The exponent
		 */
		void growthSeries_symbolExponentFromProduct(const vector< vector<short unsigned int> >& iProduct, vector<unsigned int>& iSymbol, unsigned int& iExponent) const;
		
		/*!	\fn growthSeries_symbolExponentFromProduct(const vector< vector<short unsigned int> >& iProduct, string& strSymbol, unsigned int& iExponent) const
		 * 	From a product of graphs, compute the corresponding symbol [n1, n2, ..., nk] together with the exponent.
		 * 	
		 * 	\param iProduct(const vector< vector<unsigned int> >&) The product of graphs
		 * 	\param strSymbol(string&)
		 * 	\param iExponent (u nsigned int&) The exponent
		 */
		void growthSeries_symbolExponentFromProduct(const vector< vector<short unsigned int> >& iProduct, string& strSymbol, unsigned int& iExponent) const;
		
		/*!	\fn growthSeries_parallel
		 * 	Compute the growth series
		 */
		void growthSeries_parallel();
		
		/*!	\fn growthSeries_sequential
		 * 	Compute the growth series
		 */
		void growthSeries_sequential();
		
		void growthSeries_details();
		
		/*!	\fn growthSeries_mergeTerms
		 * 	Given the parameters, compute iPolynomial/iSymbol += iTemp_polynomial/iTemp_symbol
		 * 
		 * 	\param iPolynomial(vector< mpz_class >&) First polynomial (by reference)
		 * 	\param iSymbol(vector<short unsigned int>&) First symbol (by reference)
		 * 	\param iTemp_polynomial(vector< mpz_class >) Second polynomial
		 * 	\param iTemp_symbol(const vector<short unsigned int>&) Second symbol
		 * 	\param biTemp(mpz_class) Eventually, some coefficient for the second polynomial
		 * 
		 * 	\return Nothing but the first two parameters are modified
		 */
		void growthSeries_mergeTerms(vector< mpz_class >& iPolynomial, vector<unsigned int>& iSymbol, vector< mpz_class > iTemp_polynomial, const vector<unsigned int>& iTemp_symbol, mpz_class biTemp = 1);
		
	public:
		friend ostream& operator<<(ostream& , CoxIter const &);
};

inline unsigned int iLinearizationMatrix_index(const unsigned int& i, const unsigned int& j, const unsigned int& n)
{
	return (i*(2*n - 1 - i)/2 + j);
}

inline unsigned int iLinearizationMatrix_row(const unsigned int& k, const unsigned int& n)
{
	return ((2*n+1 - iSQRTsup((2*n+1)*(2*n+1) - 8*k))/2);
}

inline unsigned int iLinearizationMatrix_col(const unsigned int& k, const unsigned int& n)
{
	unsigned int iRow(iLinearizationMatrix_row(k, n));
	return (k - (iRow * (2*n-1-iRow))/2);
}

#endif
