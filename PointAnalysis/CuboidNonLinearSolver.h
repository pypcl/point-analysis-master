#ifndef CUBOIDNONLINEARSOLVER_H
#define CUBOIDNONLINEARSOLVER_H

#include "papart.h"
#include "CuboidRelation.h"
#include "CuboidPredictor.h"
#include "CuboidSYmmetryGroup.h"

/* IPOPT */
#include "NLPFormulation.h"
#include "NLPEigenQuadFunction.h"
#include "NLPVectorExpression.h"
#include "IPOPTSolver.h"
#include <IpIpoptApplication.hpp>

#include <ANN/ANN.h>
#include <Eigen/Core>
#include <bitset>
#include <Eigen/Eigenvalues> 

class CuboidNonLinearSolver
{
public:
	CuboidNonLinearSolver(
		const std::vector<PAPart *>& _cuboids,
		const std::vector<CuboidReflectionSymmetryGroup *>& _reflection_symmetry_groups,
		const std::vector<CuboidRotationSymmetryGroup *>& _rotation_symmetry_groups,
		const Real _squared_neighbor_distance,
		const unsigned int _min_num_symmetry_point_pairs,
		const Real _symmetry_energy_term_weight);
	~CuboidNonLinearSolver();


	inline unsigned int num_total_variables() const;
	inline unsigned int num_total_cuboid_corner_variables() const;
	inline unsigned int num_total_cuboid_axis_variables() const;
	inline unsigned int num_total_reflection_symmetry_group_variables() const;
	inline unsigned int num_total_rotation_symmetry_group_variables() const;

	static NLPVectorExpression create_vector_variable(
		const std::pair<Index, Index>& _index_size_pair);

	// Pair: (index, size)
	inline std::pair<Index, Index> get_cuboid_corner_variable_index_size(
		unsigned int _cuboid_index, unsigned int _corner_index) const;
	inline std::pair<Index, Index> get_cuboid_axis_variable_index_size(
		unsigned int _cuboid_index, unsigned int _axis_index) const;
	inline std::pair<Index, Index> get_reflection_symmetry_group_variable_n_index_size(
		unsigned int _symmetry_group_index) const;
	inline std::pair<Index, Index> get_reflection_symmetry_group_variable_t_index_size(
		unsigned int _symmetry_group_index) const;
	inline std::pair<Index, Index> get_rotation_symmetry_group_variable_n_index_size(
		unsigned int _symmetry_group_index) const;
	inline std::pair<Index, Index> get_rotation_symmetry_group_variable_t_index_size(
		unsigned int _symmetry_group_index) const;  /* 获取当前旋转对称偏移向量t的结束位置 */

	inline NLPVectorExpression create_cuboid_corner_variable(
		unsigned int _cuboid_index, unsigned int _corner_index) const;
	inline NLPVectorExpression create_cuboid_axis_variable(
		unsigned int _cuboid_index, unsigned int _axis_index) const;
	inline NLPVectorExpression create_reflection_symmetry_group_variable_n(
		unsigned int _symmetry_group_index) const;
	inline NLPVectorExpression create_reflection_symmetry_group_variable_t(
		unsigned int _symmetry_group_index) const;
	inline NLPVectorExpression create_rotation_symmetry_group_variable_n(
		unsigned int _symmetry_group_index) const;
	inline NLPVectorExpression create_rotation_symmetry_group_variable_t(
		unsigned int _symmetry_group_index) const;


	void optimize(
		const Eigen::MatrixXd& _cuboid_quadratic_term,
		const Eigen::VectorXd& _cuboid_linear_term,
		const double _cuboid_constant_term,
		Eigen::VectorXd* _init_values_vec = NULL,
		const std::vector<unsigned int> *_fixed_cuboid_indices = NULL);


private:
	// Core functions.
	void create_energy_functions(
		const Eigen::MatrixXd &_quadratic_term,
		const Eigen::VectorXd &_linear_term,
		const double _constant_term,
		std::vector<NLPFunction *> &_functions);

	void add_constraints(NLPFormulation &_formulation);

	bool compute_initial_values(const Eigen::VectorXd &_input, Eigen::VectorXd &_output);

	void update(const std::vector< Number >& _values);


	// Energy functions.
	NLPFunction *create_reflection_symmetry_group_energy_function();

	void create_reflection_symmetry_group_energy_function(
		const unsigned int _symmetry_group_index,
		const std::vector<ANNpointArray>& _cuboid_ann_points,
		const std::vector<ANNkd_tree *>& _cuboid_ann_kd_tree,
		Eigen::MatrixXd& _quadratic_term,
		Eigen::VectorXd& _linear_term,
		double &_constant_term);

	NLPFunction *create_rotation_symmetry_group_energy_function();

	void create_rotation_symmetry_group_energy_function(
		const unsigned int _symmetry_group_index,
		const std::vector<ANNpointArray>& _cuboid_ann_points,
		const std::vector<ANNkd_tree *>& _cuboid_ann_kd_tree,
		NLPExpression &_expression);

	void create_cuboid_sample_point_ann_trees(
		std::vector<ANNpointArray>& _cuboid_ann_points,
		std::vector<ANNkd_tree *>& _cuboid_ann_kd_tree) const;

	void delete_cuboid_sample_point_ann_trees(
		std::vector<ANNpointArray>& _cuboid_ann_points,
		std::vector<ANNkd_tree *>& _cuboid_ann_kd_tree) const;


	// Constraint functions.
	void add_cuboid_constraints(NLPFormulation &_formulation);
	void add_reflection_symmetry_group_constraints(NLPFormulation &_formulation);
	void add_rotation_symmetry_group_constraints(NLPFormulation &_formulation);

	void add_cuboid_constraints(
		const unsigned int _cuboid_index,
		NLPFormulation &_formulation);

	void add_reflection_symmetry_group_constraints(
		const unsigned int _symmetry_group_index,
		NLPFormulation &_formulation);

	void add_reflection_constraints(
		const NLPVectorExpression& _x_variable,
		const NLPVectorExpression& _y_variable,
		const NLPVectorExpression& _n_variable,
		const NLPVectorExpression& _t_variable,
		NLPFormulation &_formulation);

	void add_single_cuboid_reflection_constraints(
		const unsigned int _cuboid_index,
		const unsigned int _symmetry_group_index,
		const unsigned int _reflection_axis_index,
		NLPFormulation &_formulation);

	void add_pair_cuboid_reflection_constraints(
		const unsigned int _cuboid_index_1,
		const unsigned int _cuboid_index_2,
		const unsigned int _symmetry_group_index,
		const unsigned int _reflection_axis_index,
		NLPFormulation &_formulation);

	void add_rotation_symmetry_group_constraints(
		const unsigned int _symmetry_group_index,
		NLPFormulation &_formulation);

	void fix_cuboid(
		const unsigned int _cuboid_index,
		NLPFormulation &_formulation);


	// Initialization function.
	void compute_cuboid_axis_values(Eigen::VectorXd &_values);
	void compute_reflection_symmetry_group_values(Eigen::VectorXd &_values);
	void compute_rotation_symmetry_group_values(Eigen::VectorXd &_values);


	// Update function.
	void update_cuboids(const std::vector< Number >& _values);
	void update_reflection_symmetry_groups(const std::vector< Number >& _values);
	void update_rotation_symmetry_groups(const std::vector< Number >& _values);


private:
	const std::vector<PAPart *>& cuboids_;
	const std::vector<CuboidReflectionSymmetryGroup *>& reflection_symmetry_groups_;
	const std::vector<CuboidRotationSymmetryGroup *>& rotation_symmetry_groups_;
	const Real squared_neighbor_distance_;  /* 0.05 * 网格直径(或半径，不确定) */
	const double symmetry_energy_term_weight_;   /* 1.0e6 */
	const unsigned int min_num_symmetric_point_pairs_;  /* 50 */

	unsigned int num_cuboids_;
	unsigned int num_reflection_symmetry_groups_;
	unsigned int num_rotation_symmetry_groups_;

	unsigned int cuboid_corner_variable_start_index_;  /* 0。包围盒顶点坐标的特征起点位置 */
	unsigned int cuboid_axis_variable_start_index_;  /* 包围盒坐标轴的特征起点位置，于所欲顶点坐标特征之后 */
	unsigned int reflection_symmetry_group_variable_start_index_;  /* 反射对称变量起点位置，于所有包围盒坐标轴特征之后 */
	unsigned int rotation_symmetry_group_variable_start_index_;  /* 旋转对称变量起点位置，于所有反射对称变量之后 */

	const unsigned int num_cuboid_corner_variables_;  /* 24 */
	const unsigned int num_cuboid_axis_variables_;  /* 3 * 3 = 9 */
	const unsigned int num_reflection_symmetry_group_variables_; /* 反射对称为3 + 1 = 4 */
	const unsigned int num_rotation_symmetry_group_variables_;  /* 旋转对称为3 + 3 = 6 */
};

#endif