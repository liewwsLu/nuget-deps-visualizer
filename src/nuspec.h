#pragma once

#include <stdexcept>
#include <string>
#include <vector>

/// Файл .nuspec повреждён.
struct NuspecError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

/// Зависимость: имя пакета и допустимый диапазон версий.
struct Dependency {
    std::string packageId;
    std::string versionRange;

    bool operator==(const Dependency& other) const {
        return packageId == other.packageId && versionRange == other.versionRange;
    }
};

/// Группа зависимостей для одной целевой платформы (пустая строка — без группы).
struct DependencyGroup {
    std::string framework;
    std::vector<Dependency> dependencies;
};

/// Зависимости из текста .nuspec, сгруппированные по целевым платформам.
std::vector<DependencyGroup> parseDependencyGroups(const std::string& xmlText);

/// Имя платформы в общем виде: «.NETStandard2.0» -> «netstandard2.0».
std::string normalizeFramework(const std::string& name);

/// Выбрать одну группу: netstandard2.0, иначе netstandard2.1, иначе без группы,
/// иначе первая. Если групп нет — пустая группа.
DependencyGroup selectGroup(const std::vector<DependencyGroup>& groups);
