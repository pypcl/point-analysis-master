#include "pointanalysis.h"

PointAnalysis::PointAnalysis(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	qRegisterMetaType<PCModel *>("PCModelPointer");
	qRegisterMetaType<Model *>("ModelPointer");
	qRegisterMetaType<PAPointCloud *>("PAPointCloud");
	qRegisterMetaType<QVector<int>>("QVectorInt");
	qRegisterMetaType<QVector<OBB *>>("OBBPointer");
	qRegisterMetaType<QVector<PAPart>>("PAPartVector");
	
	connect(ui.actionOpen, SIGNAL(triggered()), this, SLOT(load()));
	connect(ui.actionSave, SIGNAL(triggered()), this, SLOT(saveModel()));
	connect(ui.actionEstimate_Features, SIGNAL(triggered()), this, SLOT(estimateFeatures()));
	connect(ui.actionExtract_Point_Features, SIGNAL(triggered()), this, SLOT(extractPointFeatures()));
	connect(&pfe, SIGNAL(reportStatus(QString)), this, SLOT(setStatMessage(QString)));
	connect(&pfe, SIGNAL(showModel(Model *)), ui.displayGLWidget, SLOT(setModel(Model *)));
	connect(&pfe, SIGNAL(addDebugText(QString)), this, SLOT(onDebugTextAdded(QString)));
	connect(ui.actionTrain_Point_Classifier, SIGNAL(triggered()), this, SLOT(trainPointClassifier()));
	connect(ui.actionTest_PointCloud, SIGNAL(triggered()), this, SLOT(testPointCloud()));
	connect(ui.displayGLWidget, SIGNAL(addDebugText(QString)), this, SLOT(onDebugTextAdded(QString)));
	//connect(testPcThread.data(), SIGNAL(addDebugText(QString)), this, SLOT(onDebugTextAdded(QString)));
	//connect(testPcThread.data(), SIGNAL(reportStatus(QString)), this, SLOT(setStatMessage(QString)));
	connect(ui.actionCompute_sdf, SIGNAL(triggered()), this, SLOT(computeSdf()));
	connect(ui.actionNormalize_Meshes, SIGNAL(triggered()), this, SLOT(normalizeMeshes()));
	connect(ui.actionCompute_OBB, SIGNAL(triggered()), this, SLOT(computeOBB()));
	connect(ui.actionTrain_Parts_Relations, SIGNAL(triggered()), this, SLOT(trainPartRelations()));
	connect(ui.actionStructure_Inference, SIGNAL(triggered()), this, SLOT(inferStructure()));
	connect(&m_analyser, SIGNAL(addDebugText(QString)), this, SLOT(onDebugTextAdded(QString)));
	connect(&m_analyser, SIGNAL(sendOBBs(QVector<OBB *>)), ui.displayGLWidget, SLOT(setOBBs(QVector<OBB *>)));
	connect(&m_analyser, SIGNAL(sendPartsStructure(Parts_Structure_Pointer)), ui.displayGLWidget, SLOT(setPartsStructure(Parts_Structure_Pointer)));
	connect(ui.actionDebug_Parts_Relations, SIGNAL(triggered()), this, SLOT(debugPartRelations()));
	connect(ui.actionCheck_Models, SIGNAL(triggered()), this, SLOT(checkModels()));
	connect(ui.actionSave_Vertices_Labels, SIGNAL(triggered()), this, SLOT(saveVerticesLabels()));
	connect(ui.actionDownsample, SIGNAL(triggered()), this, SLOT(downSample()));
	connect(ui.symPlaneCheckBox, SIGNAL(stateChanged(int)), ui.displayGLWidget, SLOT(setDrawSymmetryPlanes(int)));
	connect(ui.symAxisCheckBox, SIGNAL(stateChanged(int)), ui.displayGLWidget, SLOT(setDrawSymmetryAxes(int)));
	connect(ui.rotateButton, SIGNAL(clicked()), this, SLOT(rotateModel()));
	connect(ui.drawOBBCheckBox, SIGNAL(stateChanged(int)), ui.displayGLWidget, SLOT(setDrawOBBs(int)));
	connect(ui.drawOBBAxesCheckBox, SIGNAL(stateChanged(int)), ui.displayGLWidget, SLOT(setDrawOBBsAxes(int)));
	connect(ui.snapshotButton, SIGNAL(clicked()), this, SLOT(snapshot()));
	//connect(ui.actionSave_Parts, SIGNAL(triggered()), this, SLOT(saveParts()));
	//connect(ui.captureButton, SIGNAL(clicked()), this, SLOT(capturePointCloud()));
	//connect(ui.stopButton, SIGNAL(clicked()), &m_openni_processor, SLOT(stop_running()));

	fe = NULL;
	trainThread = NULL;
	loadThread = NULL;
	outputThread = NULL;
	pcaThread = NULL;
	trainPartsThread = NULL;
	normalizeThread = NULL;
	sdfThread = NULL;
	trainThread = NULL;
	debugRelationThread = NULL;
	checkModelsThread = NULL;
	
	m_modelClassName = "coseg_chairs_8";
}

PointAnalysis::~PointAnalysis()
{

}

void PointAnalysis::load()
{
	if (loadThread != NULL)
	{
		if (loadThread->isRunning())
			loadThread->terminate();
		delete(loadThread);
		loadThread = NULL;
	}

	/* User choose a model file by FileDialog */
	QString filepath = QFileDialog::getOpenFileName(this, tr("Load"),
		"../../Data/LabeledDB/Chair",
		tr("Object File Format (*.off);;XYZ Point Cloud (*.xyz);;Stanford Polygon File Format (*.ply);;PTS File Format (*.pts)"));

	if (filepath.length() > 0){    /* If the user choose a valid model file path */
		QString name = Utils::getModelName(filepath);
		filename = name.toStdString();
		/* Set the status bar to inform users it is loading now */
		QString stat_msg = "Loading point cloud from " + filepath + "...";
		onDebugTextAdded(stat_msg);
		ui.statusBar->showMessage(stat_msg, 0);

		/* Start LoadThread to load the model */
		loadThread = new LoadThread(filepath.toStdString(), PHASE::TESTING, this);
		connect(loadThread, SIGNAL(loadPointsCompleted(Model *)), this, SLOT(loadCompleted(Model *)));
		connect(loadThread, SIGNAL(sendOBBs(QVector<OBB *>)), ui.displayGLWidget, SLOT(setOBBs(QVector<OBB *>)));
		loadThread->start();
	}
}

void PointAnalysis::loadCompleted(Model *model)
{
	ui.displayGLWidget->setModel(model);
	if (model->getType() == Model::ModelType::Mesh)
	{
		connect((MeshModel *)ui.displayGLWidget->getModel(), SIGNAL(addDebugText(QString)), ui.displayGLWidget, SLOT(onDebugTextAdded(QString)));
		connect((MeshModel *)ui.displayGLWidget->getModel(), SIGNAL(onLabelsChanged()), ui.displayGLWidget, SLOT(updateLabels()));
	}
	else
	{
		connect((PCModel *)ui.displayGLWidget->getModel(), SIGNAL(addDebugText(QString)), ui.displayGLWidget, SLOT(onDebugTextAdded(QString)));
		connect((PCModel *)ui.displayGLWidget->getModel(), SIGNAL(onLabelsChanged()), ui.displayGLWidget, SLOT(updateLabels()));
	}
	ui.statusBar->showMessage("Loading done.");
	onDebugTextAdded("Loading done.");

	if (loadThread != NULL && loadThread->isRunning())
		loadThread->terminate();
	delete(loadThread);
	loadThread = NULL;
}

void PointAnalysis::saveModel()
{
	if (outputThread != NULL)
	{
		if (outputThread->isRunning())
			outputThread->terminate();
		delete(outputThread);
		outputThread = NULL;
	}

	QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
		"../../Data",
		tr("Object File Format (*.off);;Stanford Polygon File Format (*.ply);;Sample Points File Format (*.pts)"));
	
	Model *model = ui.displayGLWidget->getModel();
	if (model->getType() == Model::ModelType::Mesh)
		connect((MeshModel *)model, SIGNAL(outputProgressReport(int)), &progressDialog1, SLOT(setProgressValue(int)));
	else
		connect((PCModel *)model, SIGNAL(outputProgressReport(int)), &progressDialog1, SLOT(setProgressValue(int)));
	outputThread = new OutputThread(this);
	connect(outputThread, SIGNAL(outputCompleted()), this, SLOT(saveCompleted()));
	outputThread->setOutputModel(model, fileName.toStdString());
	outputThread->start();
	progressDialog1.setProgressValue(0);
	progressDialog1.setWindowTitle("Point Cloud Save");
	progressDialog1.setMessage("Saving the point cloud, please wait for a few seconds...");
	progressDialog1.exec();
}

void PointAnalysis::saveCompleted()
{
	progressDialog1.done(0);

	if (outputThread != NULL)
	{
		if (outputThread->isRunning())
			outputThread->terminate();
		delete(outputThread);
		outputThread = NULL;
	}
}

void PointAnalysis::getProgressReport(int value)
{
	ui.mainProgressBar->setValue(value);
}

void PointAnalysis::estimateFeatures()
{
	if (fe != NULL)
	{
		delete(fe);
		fe = NULL;
	}
	qDebug() << "Estimate point features.";
	onDebugTextAdded("Estimate point features of " + QString::fromStdString(filename) + ".");
	ui.statusBar->showMessage("Estimating points features of " + QString::fromStdString(filename) + "...");
	fe = new FeatureEstimator(ui.displayGLWidget->getModel(), PHASE::TESTING, this);
	connect(fe, SIGNAL(estimateCompleted(PAPointCloud *)), this, SLOT(featureEstimateCompleted(PAPointCloud *)));
	connect(fe, SIGNAL(addDebugText(QString)), this, SLOT(onDebugTextAdded(QString)));
	fe->estimateFeatures();
}

void PointAnalysis::featureEstimateCompleted(PAPointCloud *pointcloud)
{
	qDebug() << "Point features estimation done.";
	onDebugTextAdded("Point features estimation done.");
	/* Write the features of the model to local file */
	std::string outputname = "../data/features_test/" + filename + ".csv";
	onDebugTextAdded("Save the point cloud features to file " + QString::fromStdString(outputname) + ".");
	setStatMessage("Saving the point cloud features to file" + QString::fromStdString(outputname) + "...");
	pointcloud->writeToFile(outputname.c_str());
	disconnect(fe, SIGNAL(estimateCompleted(PAPointCloud *)), this, SLOT(featureEstimateCompleted(PAPointCloud *)));
	ui.statusBar->showMessage("Point features estimation done.");
}

void PointAnalysis::setStatMessage(QString msg)
{
	ui.statusBar->showMessage(msg, 0);
}

void PointAnalysis::extractPointFeatures()
{
	pfe.execute();
}

void PointAnalysis::trainPointClassifier()
{
	if (trainThread != NULL)
	{
		if (trainThread->isRunning())
			trainThread->terminate();
		delete(trainThread);
		trainThread = NULL;
	}

	trainThread = new TrainThread(m_modelClassName, this);
	connect(trainThread, SIGNAL(finished()), this, SLOT(onTrainingCompleted()));
	connect(trainThread, SIGNAL(reportStatus(QString)), this, SLOT(setStatMessage(QString)));
	connect(trainThread, SIGNAL(addDebugText(QString)), this, SLOT(onDebugTextAdded(QString)));
	trainThread->start();
}

void PointAnalysis::onTrainingCompleted()
{
	if (trainThread != NULL)
	{
		if (trainThread->isRunning())
			trainThread->terminate();
		delete(trainThread);
		trainThread = NULL;
	}
}

void PointAnalysis::onDebugTextAdded(QString text)
{
	ui.debugTextEdit->append(text);
}

void PointAnalysis::testPointCloud()
{
	if (!testPcThread.isNull() && testPcThread->isRunning())
		testPcThread->quit();

	std::string model_path = ui.displayGLWidget->getModel()->getInputFilepath();
	std::string model_name = Utils::getModelName(model_path);

	/* Check whether the point cloud has been classified */
	std::string prediction_path = "../data/predictions/" + model_name + ".txt";
	ifstream prediction_in(prediction_path.c_str());

	if (prediction_in.is_open())    /* If the point cloud has been classified */
	{
		prediction_in.close();
		if (testPcThread.isNull())
		{
			testPcThread.reset(new TestPCThread(0, prediction_path, m_modelClassName, this));
			connect(testPcThread.data(), SIGNAL(addDebugText(QString)), this, SLOT(onDebugTextAdded(QString)));
			connect(testPcThread.data(), SIGNAL(setPCLabels(QVector<int>)), this, SLOT(onTestCompleted(QVector<int>)));
		}
		else
			testPcThread->setPredictionFilePath(prediction_path);
	}
	else    /* If the point cloud has not been classified */
	{
		if (testPcThread.isNull())
		{
			testPcThread.reset(new TestPCThread(QString::fromStdString(model_name), m_modelClassName, this));
			connect(testPcThread.data(), SIGNAL(addDebugText(QString)), this, SLOT(onDebugTextAdded(QString)));
			connect(testPcThread.data(), SIGNAL(setPCLabels(QVector<int>)), this, SLOT(onTestCompleted(QVector<int>)));
		}
		else
			testPcThread->setPcName(QString::fromStdString(model_name));
	}

	testPcThread->start();
}

void PointAnalysis::onTestCompleted(QVector<int> labels)
{
	onDebugTextAdded("Test Completed. Set the labels to point cloud model.");
	ui.displayGLWidget->getModel()->setLabels(labels);
}

void PointAnalysis::computeSdf()
{
	sdfThread = new SdfThread(m_modelClassName, this);
	connect(sdfThread, SIGNAL(addDebugText(QString)), this, SLOT(onDebugTextAdded(QString)));
	connect(sdfThread, SIGNAL(computeSdfCompleted()), this, SLOT(onComputeSdfDone()));
	sdfThread->execute();
}

void PointAnalysis::onComputeSdfDone()
{
	onDebugTextAdded("Sdf computation done.");
}

void PointAnalysis::normalizeMeshes()
{
	if (normalizeThread != NULL)
	{
		if (normalizeThread->isRunning())
			normalizeThread->terminate();
		delete(normalizeThread);
		normalizeThread = NULL;
	}

	QString file_list_path = QFileDialog::getOpenFileName(this, tr("Choose the model list file"),
		"../data",
		tr("Model List File (*.txt)"));

	normalizeThread = new NormalizeThread(m_modelClassName, this);
	connect(normalizeThread, SIGNAL(addDebugText(QString)), this, SLOT(onDebugTextAdded(QString)));
	connect(normalizeThread, SIGNAL(finished()), this, SLOT(onNormalizeDone()));
	normalizeThread->start();
}

void PointAnalysis::onNormalizeDone()
{
	if (normalizeThread != NULL)
	{
		if (normalizeThread->isRunning())
			normalizeThread->terminate();
		delete(normalizeThread);
		normalizeThread = NULL;
	}
}

void PointAnalysis::computeOBB()
{
	if (pcaThread != NULL)
	{
		if (pcaThread->isRunning())
			pcaThread->terminate();
		delete(pcaThread);
		pcaThread = NULL;
	}

	pcaThread = new PCAThread(ui.displayGLWidget->getModel(), PHASE::TRAINING, this);
	connect(pcaThread, SIGNAL(finished()), this, SLOT(onComputeOBBDone()));
	connect(pcaThread, SIGNAL(estimateOBBsCompleted(QVector<OBB *>)), ui.displayGLWidget, SLOT(setOBBs(QVector<OBB *>)));
	connect(pcaThread, SIGNAL(addDebugText(QString)), this, SLOT(onDebugTextAdded(QString)));
	connect(pcaThread, SIGNAL(sendSamples(Samples_Vec)), ui.displayGLWidget, SLOT(setSamples(Samples_Vec)));
	pcaThread->start();
}

void PointAnalysis::onComputeOBBDone()
{
	if (pcaThread != NULL)
	{
		if (pcaThread->isRunning())
			pcaThread->terminate();
		delete(pcaThread);
		pcaThread = NULL;
	}
}

void PointAnalysis::trainPartRelations()
{
	if (trainPartsThread != NULL)
	{
		if (trainPartsThread->isRunning())
			trainPartsThread->terminate();
		delete(trainPartsThread);
		trainPartsThread = NULL;
	}

	QVector<int> label_names(8);
	for (int i = 0; i < 8; i++)
		label_names[i] = i;

	trainPartsThread = new TrainPartsThread(label_names, this);
	connect(trainPartsThread, SIGNAL(addDebugText(QString)), this, SLOT(onDebugTextAdded(QString)));
	connect(trainPartsThread, SIGNAL(showModel(Model *)), ui.displayGLWidget, SLOT(setModel(Model *)));
	connect(&trainPartsThread->pcaThread, SIGNAL(estimateOBBsCompleted(QVector<OBB*>)), ui.displayGLWidget, SLOT(setOBBs(QVector<OBB *>)));
	connect(trainPartsThread, SIGNAL(finished()), this, SLOT(onTrainPartsDone()));
	/* No need "trainPartsThread->start();", the thread will start itself after construction */
}

void PointAnalysis::debugPartRelations()
{
	if (debugRelationThread != NULL)
	{
		if (debugRelationThread->isRunning())
			debugRelationThread->terminate();
		delete(debugRelationThread);
		debugRelationThread = NULL;
	}

	debugRelationThread = new DebugRelationThread(this);
	debugRelationThread->execute();
}

void PointAnalysis::onTrainPartsDone()
{
	if (trainPartsThread != NULL)
	{
		if (trainPartsThread->isRunning())
			trainPartsThread->terminate();
		delete(trainPartsThread);
		trainPartsThread = NULL;
	}
}

void PointAnalysis::inferStructure()
{
	m_analyser.setPointCloud((PCModel *)ui.displayGLWidget->getModel());
	m_analyser.execute();
}

void PointAnalysis::checkModels()
{
	if (checkModelsThread == NULL)
		checkModelsThread = new CheckModelsThread(this);

	connect(checkModelsThread, SIGNAL(showModel(Model *)), ui.displayGLWidget, SLOT(setModel(Model *)));
	connect(checkModelsThread, SIGNAL(addDebugText(QString)), this, SLOT(onDebugTextAdded(QString)));
	connect(checkModelsThread, SIGNAL(finish()), this, SLOT(checkModelsDone()));
	checkModelsThread->execute();
}

void PointAnalysis::checkModelsDone()
{
	if (checkModelsThread != NULL)
	{
		delete(checkModelsThread);
	}
}

void PointAnalysis::saveVerticesLabels()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Segmentation File"),
		"../../shape2pose/data/1_input/coseg_chairs/gt/",
		tr("Segmentation File Format (*.seg)"));

	if (fileName.length() > 0)
	{
		ui.displayGLWidget->getModel()->outputVerticesLabels(fileName.toStdString().c_str());
	}
}

void PointAnalysis::downSample()
{
	ui.displayGLWidget->getModel()->downSample();
}

void PointAnalysis::rotateModel()
{
	float angle = ui.angleInput->toPlainText().toFloat();
	float x = ui.xAxisInput->toPlainText().toFloat();
	float y = ui.yAxisInput->toPlainText().toFloat();
	float z = ui.zAxisInput->toPlainText().toFloat();

	ui.displayGLWidget->rotateModel(angle, x, y, z);
}

void PointAnalysis::snapshot()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
		"D:\\Pictures\\experiments",
		tr("PNG (*.png)"));

	if (!fileName.isEmpty())
	{
		ui.displayGLWidget->slotSnapshot(fileName.toStdString());
	}
}

void PointAnalysis::saveParts()
{
	QString directory = QFileDialog::getExistingDirectory(this, tr("Open Directory"), 
		"../data", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	if (directory.length() > 0)
		ui.displayGLWidget->getModel()->splitOutput(directory.toStdString());
}

void PointAnalysis::capturePointCloud()
{
	//m_openni_processor.start();
}