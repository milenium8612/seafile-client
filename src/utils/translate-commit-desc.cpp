#include <stdlib.h>
#include <stdio.h>
#include <QHash>
#include <QObject>
#include <QApplication>
#include <QRegExp>
#include <QStringList>

#include "utils/utils.h"
#include "translate-commit-desc.h"

namespace {

//const char *kTranslateContext = "MessageListener";

QHash<QString, QString> *verbsMap = NULL;

QHash<QString, QString>*
getVerbsMap()
{
    if (!verbsMap) {
        verbsMap = new QHash<QString, QString>;
        verbsMap->insert("Added", QObject::tr("Added"));
        verbsMap->insert("Added or modified", QObject::tr("Added or modified"));
        verbsMap->insert("Deleted", QObject::tr("Deleted"));
        verbsMap->insert("Removed", QObject::tr("Removed"));
        verbsMap->insert("Modified", QObject::tr("Modified"));
        verbsMap->insert("Renamed", QObject::tr("Renamed"));
        verbsMap->insert("Moved", QObject::tr("Moved"));
        verbsMap->insert("Added directory", QObject::tr("Added directory"));
        verbsMap->insert("Removed directory", QObject::tr("Removed directory"));
        verbsMap->insert("Renamed directory", QObject::tr("Renamed directory"));
        verbsMap->insert("Moved directory", QObject::tr("Moved directory"));
    }

    return verbsMap;
}

QString translateLine(const QString line)
{
    QString operations = ((QStringList)getVerbsMap()->keys()).join("|");
    QString pattern = QString("(%1) \"(.*)\"\\s?(and ([0-9]+) more (files|directories))?").arg(operations);

    QRegExp regex(pattern);

    if (regex.indexIn(line) < 0) {
        return line;
    }

    QString op = regex.cap(1);
    QString file_name = regex.cap(2);
    QString has_more = regex.cap(3);
    QString n_more = regex.cap(4);
    QString more_type = regex.cap(5);

    QString op_trans = getVerbsMap()->value(op, op);

    QString type, ret;
    if (has_more.length() > 0) {
        if (more_type == "files") {
            type = QObject::tr("files");
        } else {
            type = QObject::tr("directories");
        }

        QString more = QObject::tr("and %1 more").arg(n_more);
        ret = QString("%1 \"%2\" %3 %4.").arg(op_trans).arg(file_name).arg(more).arg(type);
    } else {
        ret = QString("%1 \"%2\".").arg(op_trans).arg(file_name);
    }

    return ret;
}

} // namespace


QString
translateCommitDesc(const QString& input)
{
    QString value = input;
    if (value.startsWith("Reverted repo")) {
        value.replace("repo", "library");
    }

    if (value.startsWith("Reverted library")) {
        return value.replace("Reverted library to status at", QObject::tr("Reverted library to status at"));
    } else if (value.startsWith("Reverted file")) {
        QRegExp regex("Reverted file \"(.*)\" to status at (.*)");

        if (regex.indexIn(value) >= 0) {
            QString name = regex.cap(1);
            QString time = regex.cap(2);
            return QObject::tr("Reverted file \"%1\" to status at %2.").arg(name).arg(time);
        }

    } else if (value.startsWith("Recovered deleted directory")) {
        return value.replace("Recovered deleted directory", QObject::tr("Recovered deleted directory"));
    } else if (value.startsWith("Changed library")) {
        return value.replace("Changed library name or description", QObject::tr("Changed library name or description"));
    } else if (value.startsWith("Merged") || value.startsWith("Auto merge")) {
        return QObject::tr("Auto merge by %1 system").arg(getBrand());
    }

    QStringList lines = value.split("\n");
    QStringList out;

    for (int i = 0; i < lines.size(); i++) {
        out << translateLine(lines.at(i));
    }

    return out.join("\n");
}

QString
translateComitActivitiesDesc(QString path, QString file_name, QString repo_name, QString obj_type, QString op_type)
{
    QString operation;
    QString subject;

    if (obj_type == "repo") {
        if (op_type == "create") {
            operation = QObject::tr("Created libraray");
            subject = repo_name;
        } else if (op_type == "rename") {
            operation = QObject::tr("Renamed libraray");
            subject = repo_name;
        } else if (op_type == "delete") {
            operation = QObject::tr("Deleted libraray");
            subject = repo_name;
        } else if (op_type == "recover") {
            operation = QObject::tr("Recovered libraray");
            subject = repo_name;
        } else if (op_type == "clean_up_trash") {
            operation = QObject::tr("Cleaned up trash");
            subject = file_name;
        }
    } else if (obj_type == "draft") {
            operation = QObject::tr("Published draft");
            subject = file_name;
    } else if(obj_type == "file") {
        if (op_type == "create") {
            operation = QObject::tr("Created file");
            subject = file_name;
        } else if (op_type == "rename") {
            operation = QObject::tr("Renamed file");
            subject = file_name;
        } else if (op_type == "delete") {
            operation = QObject::tr("Deleted file");
            subject = file_name;
        } else if (op_type == "recover") {
            operation = QObject::tr("Recovered delete file");
            subject = file_name;
        } else if (op_type == "move") {
            operation = QObject::tr("Moved file");
            subject = file_name;
        } else if (op_type == "edit") {
            operation = QObject::tr("Edited file");
            subject = file_name;
        }

    } else if(obj_type == "files") {
        // TODO:

    } else { //dir
        if (op_type == "create") {
            operation = QObject::tr("Created dir");
            subject = path;
        } else if (op_type == "rename") {
            operation = QObject::tr("Renamed dir");
            subject = path;
        } else if (op_type == "delete") {
            operation = QObject::tr("Deleted dir");
            subject = path;
        } else if (op_type == "recover") {
            operation = QObject::tr("Recover deleled dir");
            subject = path;
        } else if (op_type == "move") {
            operation = QObject::tr("Moved dir");
            subject = path;
        }
    }

    return QString("%1 %2").arg(operation).arg(subject);
}
