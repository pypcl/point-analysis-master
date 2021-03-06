#ifndef POINTANALYSIS_H
#define POINTANALYSIS_H

#include <QtWidgets/QMainWindow>
#include <qfiledialog.h>
#include <QVector>
#include "ui_pointanalysis.h"
#include "utils.h"
#include "pcmodel.h"
#include "outputthread.h"
#include "progressdialog1.h"
#include "loadthread.h"
#include "featureestimator.h"
#include "PAPointCloud.h"
#include "pointfeatureextractor.h"
#include "trainthread.h"
#include "testpcthread.h"
#include "sdfthread.h"
#include "normalizethread.h"
#include "pcathread.h"
#include "obb.h"
#include "trainpartsthread.h"
#include "papart.h"
#include "structureanalyser.h"

class PointAnalysis : public QMainWindow
{
	Q_OBJECT

public:
	PointAnalysis(QWidget *parent = 0);
	~PointAnalysis();

	public slots:
	void load();
	void saveModel();
	void saveCompleted();
	void loadCompleted(PCModel *model);
	void getProgressReport(int value);
	void estimateFeatures();
	void featureEstimateCompleted(PAPointCloud *cloud);
	void testPointCloud();
	void setStatMessage(QString stat);
	void extractPointFeatures();
	void trainPointClassifier();
	void onTrainingCompleted();
	void onDebugTextAdded(QString text);
	void onTestCompleted(QVector<int> labels);
	void computeSdf();
	void onComputeSdfDone();
	void normalizeMeshes();
	void onNormalizeDone();
	void computeOBB();
	void onComputeOBBDone();
	void trainPartRelations();
	void onTrainPartsDone();
	void inferStructure();

private:
	Ui::PointAnalysisClass ui;
	ProgressDialog1 progressDialog1;
	OutputThread *outputThread;
	LoadThread *loadThread;
	FeatureEstimator *fe;
	std::string filename;
	PointFeatureExtractor pfe;
	TrainThread *trainThread;
	TestPCThread testPcThread;
	SdfThread *sdfThread;
	NormalizeThread *normalizeThread;
	PCAThread *pcaThread;
	TrainPartsThread *trainPartsThread;
	StructureAnalyser m_analyser;
	std::string m_modelClassName;
};

#endif // POINTANALYSIS_H
