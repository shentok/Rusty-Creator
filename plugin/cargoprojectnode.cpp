#include "cargoprojectnode.h"

#include <QFileInfo>
#include <QDir>
#include <QFileInfoList>
#include <projectexplorer/projectnodes.h>

using namespace Rust;
using namespace Utils;
using namespace ProjectExplorer;

void populateNode(FolderNode* node, QFileInfoList excluded, QString dirPath = QString()) {
    if(dirPath.isNull())
        dirPath = node->path().toString();

    QList<FolderNode*> subDirs;
    QList<FileNode*> subFiles;

    for (QFileInfo sub: QDir(dirPath).entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::DirsFirst | QDir::Name)) {
        if(excluded.contains(sub))
            continue;
        else if(sub.isDir())
            subDirs << new FolderNode(FileName(sub), FolderNodeType, sub.fileName());
        else
            subFiles << new FileNode(FileName(sub), UnknownFileType, false);
    }

    node->addFolderNodes(subDirs);
    node->addFileNodes(subFiles);

    for (FolderNode* subDir : subDirs)
        populateNode(subDir, excluded);
}

// `projectFilePath` is the path of the Cargo.toml file. It can be seen in the
// tooltip attached to the project explorer's root item.
//
CargoProjectNode::CargoProjectNode(const FileName& projectFilePath)
    : ProjectExplorer::ProjectNode(projectFilePath)
{
    projName_ = projectFilePath.parentDir().fileName();
    QDir mainDir = QDir(projectFilePath.parentDir().toString());
    QFileInfoList excluded;
    excluded << QFileInfo(mainDir, QString::fromLatin1("target"))
             << QFileInfo(mainDir, QString::fromLatin1(".hg"))
             << QFileInfo(mainDir, QString::fromLatin1(".git"))
             << QFileInfo(mainDir, QString::fromLatin1(".svn"))
             << QFileInfo(mainDir, QString::fromLatin1(".darcs"));
    populateNode(this, excluded, mainDir.path());
}

bool CargoProjectNode::canAddSubProject(const QString &proFilePath) const
{
    Q_UNUSED(proFilePath)
    return false;
}

bool CargoProjectNode::addSubProjects(const QStringList &proFilePaths)
{
    Q_UNUSED(proFilePaths)
    return false;
}

bool CargoProjectNode::removeSubProjects(const QStringList &proFilePaths)
{
    Q_UNUSED(proFilePaths)
    return false;
}

// The name displayed on the project explorer's root item.
QString CargoProjectNode::displayName() const
{
    return projName_;
}
