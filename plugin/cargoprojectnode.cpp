#include "cargoprojectnode.h"

#include <projectexplorer/projectnodes.h>

using namespace Rust;
using namespace Utils;

// `projectFilePath` is the path of the Cargo.toml file. It can be seen in the
// tooltip attached to the project explorer's root item.
//
CargoProjectNode::CargoProjectNode(const FileName& projectFilePath)
    : ProjectExplorer::ProjectNode(projectFilePath),
      projName_(projectFilePath.parentDir().fileName())
{
}

bool CargoProjectNode::canAddSubProject(const QString &proFilePath) const
{
    Q_UNUSED(proFilePath)
    // STUB
    return false;
}

bool CargoProjectNode::addSubProjects(const QStringList &proFilePaths)
{
    Q_UNUSED(proFilePaths)
    // STUB
    return false;
}

bool CargoProjectNode::removeSubProjects(const QStringList &proFilePaths)
{
    Q_UNUSED(proFilePaths)
    // STUB
    return false;
}

// The name displayed on the project explorer's root item.
QString CargoProjectNode::displayName() const
{
    return projName_;
}
