#ifndef CARGOPROJECT_H
#define CARGOPROJECT_H

#include <projectexplorer/project.h>

#include <utils/fileutils.h>

#include <QDir>
#include <QFileInfoList>
#include <QString>
#include <QScopedPointer>

namespace ProjectExplorer { class FolderNode; }
namespace Rust { class CargoProjectManager; }
namespace Rust { class CargoProjectNode; }

class QFileSystemWatcher;

namespace Rust {

/// Represents one entry in the "Projects" navigator view.
///
class CargoProject : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    CargoProject(CargoProjectManager* projectManager, QString projectFileName);
    ~CargoProject();
    // ProjectExplorer::Project interface
    virtual QString displayName() const override;
    virtual ProjectExplorer::IProjectManager* projectManager() const override;
    virtual ProjectExplorer::ProjectNode* rootProjectNode() const override;
    virtual QStringList files(FilesMode fileMode) const override;

private slots:
    /// This slot is connected to the `QFileSystemWatcher::directoryChanged`
    /// signal. One signal/slot connection exists for each node that represents
    /// a directory in the tree.
    ///
    /// It modifies the tree content so that the nodes match the content of the
    /// filesystem.
    ///
    void updateDirContent(const QString&);

private:
    Q_DISABLE_COPY(CargoProject)

    /// Almost all `FolderNode` nodes represent a directory, and their
    /// `ProjectNode::path` method returns the path of this directory. The
    /// exception is the root node, whose `path` method returns the Cargo.toml
    /// file path.
    ///
    /// In many circumstances, we want to consider the root just like a regular
    /// directory node, whose path is the Cargo.toml's parent directory. That's
    /// what this method does.
    ///
    const Utils::FileName realDir(ProjectExplorer::FolderNode* node) const;

    enum SearchState { SearchStop, SearchContinue };

    /// Implementation detail of `updateDirContent`. It searches recursively in
    /// the tree, for the node whose `path` matches the one that changed.
    ///
    /// Returns `SearchStop` to signal that the one matching directory was
    /// found, and thus indicate that the recursion must stop.
    ///
    SearchState updateDirContentRecursive(ProjectExplorer::FolderNode* node, const Utils::FileName& dir);

    /// Calls `updateFiles` and `updateDirs`
    ///
    void updateFilesAndDirs(ProjectExplorer::FolderNode* node);

    /// Perform a diff between the `FileNode` children that are already known,
    /// and the files on the filesystem. Add or remove nodes accordingly.
    ///
    /// Ignores the paths listed in `excludedPaths_`.
    ///
    void updateFiles(ProjectExplorer::FolderNode* node);

    /// Perform a diff between the `FolderNode` children that are already
    /// known, and the subdirectories on the filesystem. Add or remove nodes
    /// accordingly.
    ///
    /// Ignores the paths listed in `excludedPaths_`.
    ///
    void updateDirs(ProjectExplorer::FolderNode* node);

    /// Called with an empty FolderNode as an argument, to recursively populate
    /// its content based on the filesystem content.
    ///
    /// This method is also responsible for adding the path of the node to the
    /// list of the directories watched by the `QFileSystemWatcher`.
    ///
    void populateNode(ProjectExplorer::FolderNode* node);

    static QFileInfoList excludedPaths(const QDir& mainDir);

    CargoProjectManager* projectManager_;
    QDir projectDir_;
    QScopedPointer<CargoProjectNode> rootNode_;

    /// The watcher used to watch the full tree hierarchy. Owned by
    /// `CargoProjectNode`.
    ///
    QFileSystemWatcher* fsWatcher_;

    /// List of absolute paths we don't want to display in the tree.
    ///
    QFileInfoList excludedPaths_;
};

}

#endif
