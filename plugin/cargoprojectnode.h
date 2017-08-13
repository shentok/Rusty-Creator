#ifndef CARGOPROJECTNODE_H
#define CARGOPROJECTNODE_H

#include "projectexplorer/projectnodes.h"

#include <QObject>
#include <QString>

class QStringList;

namespace Utils { class FileName; }

namespace Rust {

/// Represents a root of a project, in the "Projects" navigator view. Under
/// this root, there are regular (non subclassed) `FolderNode` and `FileNode`
/// nodes.
///
/// Except for the root, all nodes represent real directories and files. The
/// content of the tree is a 1:1 view of the filesystem content, and is
/// synchronized using a `QFileSystemWatcher`.
///
class CargoProjectNode : public QObject, public ProjectExplorer::ProjectNode
{
    Q_OBJECT

public:
    CargoProjectNode(const Utils::FileName& projectFilePath);

    // ProjectExplorer::ProjectNode interface
    virtual bool canAddSubProject(const QString &proFilePath) const override;
    virtual bool addSubProjects(const QStringList &proFilePaths) override;
    virtual bool removeSubProjects(const QStringList &proFilePaths) override;
    virtual QString displayName() const override;

private:
    /// Simple name of the project's root directory
    ///
    QString projName_;
};

}

#endif
