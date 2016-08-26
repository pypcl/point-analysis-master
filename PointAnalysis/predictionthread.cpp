#include "predictionthread.h"

PredictionThread::PredictionThread(QObject *parent)
	: QThread(parent), m_is_clean(true), m_use_symmetry(true)
{
	qRegisterMetaType<QMap<int, int>>("PartsPicked");
}

PredictionThread::PredictionThread(EnergyFunctions *energy_functions, Part_Candidates part_candidates, 
	QVector<int> label_names, bool use_symmetry, QObject *parent)
	: QThread(parent), m_is_clean(true), m_use_symmetry(use_symmetry)
{
	m_energy_functions = energy_functions;
	m_ncandidates = part_candidates.size();
	m_part_candidates = Part_Candidates(part_candidates);
	m_label_names = label_names;

	qRegisterMetaType<QMap<int, int>>("PartsPicked");

	const int nodeNum = m_ncandidates;   /* the number of nodes */
	connect(this, SIGNAL(testSignal), this, SLOT(onGetTest()));
}

PredictionThread::~PredictionThread()
{
	clean();
	for (QVector<UnaryTermThread *>::iterator it = m_unary_threads.begin(); it != m_unary_threads.end(); ++it)
	{
		UnaryTermThread * thread = *it;
		if (thread != NULL)
		{
			if (thread->isRunning())
				thread->terminate();
			delete(thread);
			thread = NULL;
		}
	}
	m_unary_threads.clear();

	for (QVector<PairwiseTermThread *>::iterator it = m_pairwise_threads.begin(); it != m_pairwise_threads.end(); ++it)
	{
		PairwiseTermThread * thread = *it;
		if (thread != NULL)
		{
			if (thread->isRunning())
				thread->terminate();
			delete(thread);
			thread = NULL;
		}
	}
	m_pairwise_threads.clear();
}

void PredictionThread::clean()
{
	if (mrf != NULL)
	{
		delete(mrf);
		mrf = NULL;
	}
	if (nodes != NULL)
	{
		delete(nodes);
		nodes = NULL;
	}

	m_is_clean = true;
}

void PredictionThread::run()
{
	/*predictLabelsAndOrientations();
	clean();*/
	singleThreadOptimize();
}

void PredictionThread::singleThreadOptimize()
{
	using namespace std;
	ofstream e_out("../data/pairwise_potentials.csv");
	ofstream u_out("../data/unary_potentials.csv");

	MRFEnergy<TypeGeneral>* mrf_s;
	MRFEnergy<TypeGeneral> *mrf_bp;
	MRFEnergy<TypeGeneral>::NodeId* nodes_s;
	MRFEnergy<TypeGeneral>::NodeId *nodes_bp;
	MRFEnergy<TypeGeneral>::Options options;
	MRFEnergy<TypeGeneral>::Options options_bp;
	TypeGeneral::REAL energy, lowerBound;

	const int labelNum = m_label_names.size();   /* the number of labels */
	const int nodeNum = m_ncandidates;   /* the number of nodes */

	mrf_s = new MRFEnergy<TypeGeneral>(TypeGeneral::GlobalSize());
	nodes_s = new MRFEnergy<TypeGeneral>::NodeId[nodeNum];
	mrf_bp = new MRFEnergy<TypeGeneral>(TypeGeneral::GlobalSize());
	nodes_bp = new MRFEnergy<TypeGeneral>::NodeId[nodeNum];

	TypeGeneral::REAL *D = new TypeGeneral::REAL[labelNum];
	TypeGeneral::REAL *V = new TypeGeneral::REAL[labelNum * labelNum];

	cout << "Set unary potentials." << endl;
	for (int i = 0; i < nodeNum; i++)
	{
		u_out << i << endl;

		PAPart cand = m_part_candidates[i];
		string unary_str = "Node_" + to_string(cand.getClusterNo()) + "_" + to_string(i) + ": ";

		Eigen::VectorXf unary_vec(labelNum);
		for (int j = 0; j < labelNum; j++)
		{
			D[j] = m_energy_functions->Epnt(&cand, j, m_use_symmetry);
			unary_vec[j] = D[j];

			if (j < labelNum - 1)
				u_out << D[j] << ",";
			else
				u_out << D[j] << endl;
		}
		
		nodes_s[i] = mrf_s->AddNode(TypeGeneral::LocalSize(labelNum), TypeGeneral::NodeData(D));
		nodes_bp[i] = mrf_bp->AddNode(TypeGeneral::LocalSize(labelNum), TypeGeneral::NodeData(D));

		cout << unary_str << endl;
		cout << unary_vec.transpose() << endl;
	}
	//e_out << endl;
	cout << "done." << endl;

	cout << "Set pairwise potentials." << endl;
	for (int i = 0; i < nodeNum; i++)
	{
		PAPart cand1 = m_part_candidates[i];
		int point_cluster_no_1 = cand1.getClusterNo();

		for (int j = i + 1; j < nodeNum; j++)
		{
			PAPart cand2 = m_part_candidates[j];
			int point_cluster_no_2 = cand2.getClusterNo();

			PAPartRelation relation(cand1, cand2);

			//e_out << "Node_" << cand1.getClusterNo() << "_" << i << " - Node_" << cand2.getClusterNo() << "_" << j << ":" << endl;
			e_out << i << "," << j << endl;

			//e_out << ",";
			/*for (int idx = 0; idx < labelNum; idx++)
			{
				if (idx < labelNum - 1)
					e_out << idx << ",";
				else
					e_out << idx << endl;
			}*/

			for (int m = 0; m < labelNum; m++)
			{
				//e_out << m << ",";
				for (int n = 0; n < labelNum; n++)
				{
					V[m + n * labelNum] = m_energy_functions->Epair(relation, point_cluster_no_1, point_cluster_no_2, m, n);
					//e_out << "V(" << m << ", " << n << ")=" << V[m + n * labelNum] << "\t";
					if (n < labelNum - 1)
						e_out << V[m + n * labelNum] << ",";
					else
						e_out << V[m + n * labelNum] << endl;
				} 
			}
			
			mrf_s->AddEdge(nodes_s[i], nodes_s[j], TypeGeneral::EdgeData(TypeGeneral::GENERAL, V));
			mrf_bp->AddEdge(nodes_bp[i], nodes_bp[j], TypeGeneral::EdgeData(TypeGeneral::GENERAL, V));
		}
	}
	cout << "done." << endl;
	e_out.close();
	u_out.close();

	/* Function below is optional - it may help if, for example, nodes are added in a random order */
	//mrf->SetAutomaticOrdering();

	/* Execute TRW-S algorithm */
	cout << "Do TRW_S algorithm." << endl;
	options.m_iterMax = 100; // maximum number of iterations
	options.m_printIter = 10;
	options.m_printMinIter = 0;
	mrf_s->ZeroMessages();
	mrf_s->AddRandomMessages(0, 0.0, 1.0);
	mrf_s->Minimize_TRW_S(options, lowerBound, energy);
	cout << "Minimization done." << endl;

	/* Execute BP algorithm */
	cout << "Do BP algorithm." << endl;
	TypeGeneral::REAL energy_bp;
	options_bp.m_iterMax = 100;
	options_bp.m_printIter = 10;
	options_bp.m_printMinIter = 0;
	mrf_bp->ZeroMessages();
	mrf_bp->AddRandomMessages(0, 0.0, 1.0);
	mrf_bp->Minimize_BP(options_bp, energy_bp);
	cout << "Minimization done." << endl;

	MRFEnergy<TypeGeneral>* mrf_optimized;
	if (energy < energy_bp)
	{
		cout << "TRW_S algorithm is more optimized. Energy = " << energy << endl;
		mrf_optimized = mrf_s;
	}
	else
	{
		cout << "BP algorithm is more optimized. Energy = " << energy_bp << endl;
		mrf_optimized = mrf_bp;
	}

	/* Read soluntions */
	std::vector<int> labels(nodeNum);    /* The i-th component of labels represents the label assigned to the i-th candidate node */
	QMap<int, int> parts_picked;  /* The parts picked out of candidates. key is part label, value is the index of candidate in parts_candidates*/
	int null_label = m_energy_functions->getNullLabelName();

	for (int i = 0; i < nodeNum; i++)
	{
		labels[i] = mrf_optimized->GetSolution(nodes_bp[i]);
		if (labels[i] != null_label)
		{
			parts_picked.insert(labels[i], i);
			std::cout << "Part label " << labels[i] << " corresponds to Candidate-" << i << std::endl;
		}
	}

	emit predictionDone(parts_picked, labels);
}

void PredictionThread::predictLabelsAndOrientations()
{
	MRFEnergy<TypeGeneral>::Options options;
	TypeGeneral::REAL energy, lowerBound;

	const int nodeNum = m_ncandidates;   /* the number of nodes */
	const int labelNum = m_label_names.size();   /* the number of labels */

	/* Function below is optional - it may help if, for example, nodes are added in a random order */
	//mrf->SetAutomaticOrdering();

	/* Execute TRW-S algorithm */
	options.m_iterMax = 100; // maximum number of iterations
	//options.m_printIter = 10;
	//options.m_printMinIter = 0;
	//mrf->ZeroMessages();
	//mrf->AddRandomMessages(0, 0.0, 1.0);
	mrf->Minimize_TRW_S(options, lowerBound, energy);

	/* Read soluntions */
	std::vector<int> labels(nodeNum);
	QMap<int, int> parts_picked;  /* The parts picked out of candidates. key is part label, value is the index of candidate in parts_candidates*/
	int null_label = m_energy_functions->getNullLabelName();

	for (int i = 0; i < nodeNum; i++)
	{
		labels[i] = mrf->GetSolution(nodes[i]);
		if (labels[i] != null_label)
		{
			//assert(parts_picked.contains(labels[i]));
			if (parts_picked.contains(labels[i]))
				qDebug() << "parts_picked.contains(labels[i])!";
			else
				parts_picked.insert(labels[i], i);
			std::cout << "Part label " << labels[i] << " corresponds to Candidate-" << i << std::endl;
		}
	}

	emit predictionDone(parts_picked, labels);
}

void PredictionThread::execute()
{
	if (!m_is_clean)
		clean();

	/* Construct energy */
	const int NUM_OF_SUBTHREADS = 8;
	const int labelNum = m_label_names.size();   /* the number of labels */
	const int nodeNum = m_ncandidates;   /* the number of nodes */
	int num_of_classes = labelNum - 1;    /* '-1' is to remove the null label */

	mrf = new MRFEnergy<TypeGeneral>(TypeGeneral::GlobalSize());
	nodes = new MRFEnergy<TypeGeneral>::NodeId[nodeNum];
	m_is_clean = false;

	/* Create 8 subthreads to set unary potentials */
	int num_of_rounds, candidates_num_per_thread;
	if (nodeNum > NUM_OF_SUBTHREADS)
	{
		num_of_rounds = NUM_OF_SUBTHREADS;
		candidates_num_per_thread = nodeNum / NUM_OF_SUBTHREADS;
	}
	else
	{
		num_of_rounds = nodeNum;
		candidates_num_per_thread = 1;
	}

	m_unary_threads.resize(num_of_rounds);
	m_unary_threads.fill(NULL);
	unfinished_unary_threads = num_of_rounds;

	for (int i = 0; i < num_of_rounds; i++)
	{
		int start = i * candidates_num_per_thread;
		int end;
		if (i < num_of_rounds - 1)
			end = (i + 1) * candidates_num_per_thread - 1;
		else
			end = nodeNum - 1;

		UnaryTermThread *unary_thread = new UnaryTermThread(i, m_part_candidates, start, end, m_energy_functions, m_label_names, this);
		connect(unary_thread, SIGNAL(computeDone(int, int, Unary_Potentials)), this, SLOT(onGetUnaryPotentials(int, int, Unary_Potentials)));
		m_unary_threads[i] = unary_thread;
		unary_thread->start();
	}

	/* Test time of adding pairwise potentials */
	//long start_time = Utils::getCurrentTime();
	//PAPart part1 = m_part_candidates.first();
	//PAPart part2 = m_part_candidates.last();
	//PAPartRelation relation(part1, part2);
	//std::cout << "Time test start." << std::endl;
	//for (int i = 0; i < 1; i++)
	//{
	//	double value = m_energy_functions->Epair(relation, part1.getClusterNo(), part2.getClusterNo(), 0, 5);
	//}
	//long end_time = Utils::getCurrentTime();
	//std::cout << "Duration = " << end_time - start_time << " ms." << std::endl;
}

void PredictionThread::onGetUnaryPotentials(int id, int start_idx, Unary_Potentials unary_potentials)
{
	qDebug("Received unary potentials of Node-%d to Node-%d from UnaryTermThread-%d.", start_idx, start_idx + unary_potentials.size() - 1, id);
	
	int labelNum = m_label_names.size();
	int count = 0;
	for (QVector<double *>::iterator it = unary_potentials.begin(); it != unary_potentials.end(); ++it)
	{
		TypeGeneral::REAL *D = *it;
		int node_idx = start_idx + count;
		nodes[node_idx] = mrf->AddNode(TypeGeneral::LocalSize(labelNum), TypeGeneral::NodeData(D));

		QString unary_potential_str = "Add Node_" + QString::number(node_idx) + ": ";
		for (int j = 0; j < labelNum - 1; j++)
			unary_potential_str.append(QString::number(D[j]) + " ");
		unary_potential_str.append(QString::number(D[labelNum - 1]));
		qDebug() << unary_potential_str;

		count++;
		delete(D);
	}

	unfinished_unary_threads--;

	/* If it is the last thread to compute unary potentials, then start threads to compute pairwise potentials */
	if (unfinished_unary_threads == 0)
	{
		qDebug() << "The unary potentials setting done.";

		const int NUM_OF_SUBTHREADS = 16;
		int num_of_cands = m_part_candidates.size();
		int num_of_units = (int)((1.0 + (float)NUM_OF_SUBTHREADS) * (float)NUM_OF_SUBTHREADS / 2.0);
		float unit = (float)num_of_cands / (float)num_of_units;

		m_pairwise_threads.resize(NUM_OF_SUBTHREADS);
		m_pairwise_threads.fill(NULL);
		unfinished_pairwise_threads = NUM_OF_SUBTHREADS;

		int start_idx = 0;
		int i;
		start_time = Utils::getCurrentTime();
		for (i = 0; i < NUM_OF_SUBTHREADS && start_idx < num_of_cands; i++)
		{
			int end;
			if (i != NUM_OF_SUBTHREADS - 1)
			{
				int n = (int)((i + 1) * unit);
				if (n < 1)
					n = 1;

				end = start_idx + n - 1;
				if (end > num_of_cands - 1)
					end = num_of_cands - 1;
			}
			else
				end = num_of_cands - 1;

			PairwiseTermThread *thread = new PairwiseTermThread(i, m_part_candidates, start_idx, end, m_energy_functions, m_label_names, this);
			connect(thread, SIGNAL(computeDone(int, int, Pairwise_Potentials)), this, SLOT(onGetPairwisePotentials(int, int, Pairwise_Potentials)));
			m_pairwise_threads[i] = thread;
			thread->start();

			start_idx = end + 1;
		}
		if (i < NUM_OF_SUBTHREADS)
		{
			m_pairwise_threads.resize(i);
			unfinished_pairwise_threads = i;
		}
	}
}

void PredictionThread::onGetPairwisePotentials(int id, int start_idx, Pairwise_Potentials pairwise_potentials)
{
	qDebug("Received pairwise potentials of Node-%d to Node-%d from PairwiseTermThread-%d.", start_idx, start_idx + pairwise_potentials.size() - 1, id);

	int outter_count = 0;
	for (Pairwise_Potentials::iterator outter_it = pairwise_potentials.begin(); outter_it != pairwise_potentials.end(); ++outter_it)
	{
		QVector<double *> potentials = *outter_it;
		int first_cand_idx = start_idx + outter_count;

		int inner_count = 1;
		for (QVector<double *>::iterator inner_it = potentials.begin(); inner_it != potentials.end(); ++inner_it)
		{
			double * V = *inner_it;
			int second_cand_idx = first_cand_idx + inner_count;

			mrf->AddEdge(nodes[first_cand_idx], nodes[second_cand_idx], TypeGeneral::EdgeData(TypeGeneral::GENERAL, V));

			inner_count++;
			delete(V);
		}

		outter_count++;
	}

	unfinished_pairwise_threads--;

	if (unfinished_pairwise_threads == 0)
	{
		qDebug() << "The pairwise potentials setting done.";
		end_time = Utils::getCurrentTime();
		int duration = end_time - start_time;
		qDebug("Time spent: %d ms.", duration);
		start();
	}
}