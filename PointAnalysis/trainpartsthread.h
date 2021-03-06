#ifndef TRAINPARTSTHREAD_H
#define TRAINPARTSTHREAD_H

#include <QThread>
#include <iostream>
#include <QVector>
#include <QDebug>
#include <QMap>
#include <QList>
#include <QPair>
#include <Eigen\Core>
#include <mlpack\core.hpp>
#include <mlpack\core\dists\gaussian_distribution.hpp>
#include <string>
#include <fstream>
#include "pcmodel.h"
#include "loadthread.h"
#include "pcathread.h"
#include "obb.h"
#include "papartrelation.h"
#include "papart.h"
#include "utils.h"

class TrainPartsThread : public QThread
{
	Q_OBJECT

public:
	TrainPartsThread(QObject *parent = 0);
	~TrainPartsThread();
	PCAThread pcaThread;

	public slots:
	void receiveModel(PCModel *pc);
	void receiveParts(QVector<PAPart> parts);

signals:
	void addDebugText(QString text);
	void showModel(PCModel *pc); 

protected:
	void run();

private:
	QList<std::string> file_list;
	LoadThread loadThread;
	QMap<QPair<int, int>, QList<PAPartRelation>> partRelations;
	QMap<QPair<int, int>, Eigen::VectorXd> mean_vecs;
	QMap<QPair<int, int>, Eigen::MatrixXd> cov_mats;
	int currentId;
	std::string class_name;

	void analyseProbPartModel();
	void savePartRelationPriors();
};

#endif // TRAINPARTSTHREAD_H
