#include "cargoproject.h"

#include "cargoprojectnode.h"
#include "cargoprojectmanager.h"

#include <QFileSystemWatcher>

using namespace ProjectExplorer;
using namespace Rust;
using namespace Utils;

CargoProject::CargoProject(CargoProjectManager* projectManager, QString projectFileName)
    : projectManager_(projectManager),
      projectDir_(QFileInfo(projectFileName).dir()),
      rootNode_(new CargoProjectNode(Utils::FileName::fromString(projectFileName))),
      fsWatcher_(new QFileSystemWatcher(this)),
      excludedPaths_(excludedPaths(projectDir_))
{
    // Hack: `QFileSystemWatcher` does not work well on Windows. When a folder
    // is being watched, and if the user tries to delete its parent directory
    // using the Windows file explorer, then a "Folder access denied" error appears
    // and the folder cannot be deleted.
    //
    // To work around the problem, we force `QFileSystemWatcher` to use the
    // polling strategy (as opposed to the native filesystem watching API),
    // which is more resource intensive and less reactive, but does not cause
    // this problem.
    //
    // https://bugreports.qt.io/browse/QTBUG-2331
    // https://bugreports.qt.io/browse/QTBUG-38118
    // https://bugreports.qt.io/browse/QTBUG-7905
    //
    #ifdef Q_OS_WIN
    fsWatcher_->setObjectName(QString::fromLatin1("_qt_autotest_force_engine_poller"));
    #endif

    connect(fsWatcher_, SIGNAL(directoryChanged(QString)),
            this, SLOT(updateDirContent(QString)));

    populateNode(rootNode_.data());
}

CargoProject::~CargoProject()
{
}

// The name of the project, that can be seen in the welcome project list,
// and in the project root's contextual menu.
QString CargoProject::displayName() const
{
    return projectDir_.dirName();
}

ProjectExplorer::IProjectManager* CargoProject::projectManager() const
{
    return projectManager_;
}

// The returned object must be the same on each call to this method.
ProjectExplorer::ProjectNode* CargoProject::rootProjectNode() const
{
    return rootNode_.data();
}

QStringList CargoProject::files(ProjectExplorer::Project::FilesMode fileMode) const
{
    Q_UNUSED(fileMode)
    // STUB
    return QStringList();
}

void CargoProject::updateDirContent(const QString &dir)
{
    updateDirContentRecursive(rootNode_.data(), FileName::fromString(dir));
}

const FileName CargoProject::realDir(FolderNode* node) const
{
    return (node->asProjectNode())
            ? FileName::fromString(projectDir_.path())
            : node->filePath();
}

CargoProject::SearchState
CargoProject::updateDirContentRecursive(FolderNode* node,
                                        const FileName& dir)
{
    const FileName currentDir = realDir(node);

    if (dir == currentDir) {
        updateFilesAndDirs(node);
        return SearchStop;
    }

    if (dir.isChildOf(currentDir)) {
        for (FolderNode* subDir: node->subFolderNodes()) {
            SearchState state = updateDirContentRecursive(subDir, dir);
            if (state == SearchStop)
                return SearchStop;
        }
    }

    return SearchContinue;
}

void CargoProject::updateFilesAndDirs(FolderNode* node)
{
    updateFiles(node);
    updateDirs(node);
}

void CargoProject::updateFiles(FolderNode* node)
{
    QSet<FileName> knownFiles;
    QMap<FileName, FileNode*> knownFilesWithNodes;
    {
        for(FileNode* f: node->fileNodes()) {
            knownFiles << f->filePath();
            knownFilesWithNodes.insert(f->filePath(), f);
        }
    }

    QSet<FileName> allFiles;
    {
        for (QFileInfo sub: QDir(realDir(node).toString()).entryInfoList(QDir::Files)) {
            if(excludedPaths_.contains(sub))
                continue;
            else
                allFiles << FileName(sub);
        }
    }

    QSet<FileName> newFiles = allFiles - knownFiles;
    QSet<FileName> obsoleteFiles = knownFiles - allFiles;

    QList<FileNode*> fileNodesToAdd;
    {
        for(const FileName& f: newFiles)
            fileNodesToAdd << new FileNode(FileName(f), UnknownFileType, false);
    }
    node->addFileNodes(fileNodesToAdd);

    QList<FileNode*> fileNodesToRemove;
    {
        for(const FileName& f: obsoleteFiles)
            fileNodesToRemove << knownFilesWithNodes[f];
    }
    node->removeFileNodes(fileNodesToRemove);
}

void CargoProject::updateDirs(FolderNode* node)
{
    QSet<FileName> knownDirs;
    QMap<FileName, FolderNode*> knownDirsWithNodes;
    {
        for(FolderNode* f: node->subFolderNodes()) {
            knownDirs << f->filePath();
            knownDirsWithNodes.insert(f->filePath(), f);
        }
    }

    QSet<FileName> allDirs;
    {
        for (QFileInfo sub: QDir(realDir(node).toString()).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            if(excludedPaths_.contains(sub))
                continue;
            else
                allDirs << FileName(sub);
        }
    }

    QSet<FileName> newDirs = allDirs - knownDirs;
    QSet<FileName> obsoleteDirs = knownDirs - allDirs;

    QList<FolderNode*> folderNodesToAdd;
    {
        for(const FileName& f: newDirs)
            folderNodesToAdd << new FolderNode(FileName(f), FolderNodeType, f.fileName());
    }
    node->addFolderNodes(folderNodesToAdd);
    for(FolderNode* newDir: folderNodesToAdd)
        populateNode(newDir);

    QList<FolderNode*> folderNodesToRemove;
    {
        for(const FileName& f: obsoleteDirs)
            folderNodesToRemove << knownDirsWithNodes[f];
    }
    node->removeFolderNodes(folderNodesToRemove);
}

void CargoProject::populateNode(FolderNode* node)
{
    Q_ASSERT(node->fileNodes().empty() && node->subFolderNodes().empty());

    QString dirPath = realDir(node).toString();

    fsWatcher_->addPath(dirPath);

    QList<FolderNode*> subDirs;
    QList<FileNode*> subFiles;

    for (QFileInfo sub: QDir(dirPath).entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
        if(excludedPaths_.contains(sub))
            continue;
        else if(sub.isDir())
            subDirs << new FolderNode(FileName(sub), FolderNodeType, sub.fileName());
        else
            subFiles << new FileNode(FileName(sub), UnknownFileType, false);
    }

    node->addFolderNodes(subDirs);
    node->addFileNodes(subFiles);

    for (FolderNode* subDir : subDirs)
        populateNode(subDir);
}

QFileInfoList CargoProject::excludedPaths(const QDir &mainDir)
{
    return QFileInfoList()
            << QFileInfo(mainDir, QString::fromLatin1("target"))
            << QFileInfo(mainDir, QString::fromLatin1(".hg"))
            << QFileInfo(mainDir, QString::fromLatin1(".git"))
            << QFileInfo(mainDir, QString::fromLatin1(".svn"))
            << QFileInfo(mainDir, QString::fromLatin1(".darcs"));
}
