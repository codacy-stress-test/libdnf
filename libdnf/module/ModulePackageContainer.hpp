/*
 * Copyright (C) 2018 Red Hat, Inc.
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef LIBDNF_MODULEPACKAGECONTAINER_HPP
#define LIBDNF_MODULEPACKAGECONTAINER_HPP

#include "libdnf/dnf-utils.h"
#include "ModulePackage.hpp"
#include "libdnf/nsvcap.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <set>
#include <stdexcept>

namespace libdnf {

struct ModulePackageContainer
{
public:
    enum class ModuleState {
        UNKNOWN,
        ENABLED,
        DISABLED,
        DEFAULT,
        INSTALLED
    };
    
    enum class ModuleErrorType {
        NO_ERROR = 0,
        INFO,
        /// Error in module defaults detected during resolvement of module dependencies
        ERROR_IN_DEFAULTS,
        /// Error detected during resolvement of module dependencies
        ERROR,
        /// Error detected during resolvement of module dependencies - Unexpected error!!!
        CANNOT_RESOLVE_MODULES,
        CANNOT_RESOLVE_MODULE_SPEC,
        CANNOT_ENABLE_MULTIPLE_STREAMS,
        CANNOT_MODIFY_MULTIPLE_TIMES_MODULE_STATE,
        /// Problem with latest modules during resolvement of module dependencies
        ERROR_IN_LATEST
    };
    
    struct Exception : public std::runtime_error
    {
        explicit Exception(const std::string &what) : runtime_error(what) {}
    };

    struct NoModuleException : public Exception
    {
        explicit NoModuleException(const std::string &moduleName) : Exception("No such module: " + moduleName) {}
    };

    struct NoStreamException : public Exception
    {
        explicit NoStreamException(const std::string &moduleStream) : Exception("No such stream: " + moduleStream) {}
    };

    struct EnabledStreamException : public Exception
    {
        explicit EnabledStreamException(const std::string &moduleName) : Exception("No enabled stream for module: " + moduleName) {}
    };

    struct EnableMultipleStreamsException : public Exception
    {
        explicit EnableMultipleStreamsException(const std::string & moduleName);
    };

    struct ConflictException : public Exception
    {
        explicit ConflictException(const std::string &what) : Exception(what) {}
    };

    struct ResolveException : public Exception
    {
        explicit ResolveException(const std::string &what) : Exception(what) {}
    };

    explicit ModulePackageContainer(bool allArch, std::string installRoot, const char * arch,
                                    const char * persistDir = nullptr);
    ~ModulePackageContainer();

    void add(const std::string & fileContent, const std::string & repoID);

    /**
     * @brief Can raise ModulePackageContainer::ConflictException
     *
     */
    void add(DnfSack * sack);

    /**
     * @brief Can raise ModulePackageContainer::ConflictException
     *
     */
    void addDefaultsFromDisk();
    void moduleDefaultsResolve();
    Id addPlatformPackage(const std::string &osReleasePath, const char *  platformModule);
    Id addPlatformPackage(DnfSack * sack,
        const std::vector<std::string> & osReleasePath, const char * platformModule);
    /// DEPRECATED
    void createConflictsBetweenStreams();

    /**
     * @brief Return true if no module package in container
     *
     * @return bool
     */
    bool empty() const noexcept;

    /**
    * @brief Can throw std::out_of_range exception
    */
    ModulePackage * getModulePackage(Id id);
    std::vector<ModulePackage *> getModulePackages();
    std::vector<std::vector<std::vector<ModulePackage *>>> getLatestModulesPerRepo(
        ModuleState moduleFilter, std::vector<ModulePackage *> modulePackages);

    /**
    * @brief Return all latest ModulePackages for each module Name, stream, context and architecture. In case of
    * multiple latest packages, all will be returned. When activeOnly is true, it returns only the latest active
    * packages.
    *
    * @return std::vector<ModulePackage *>
    */
    std::vector<ModulePackage *> getLatestModules(const std::vector<ModulePackage *> modulePackages, bool activeOnly);

    ModulePackage * getLatestModule(std::vector<ModulePackage *> modulePackages, bool activeOnly);

    std::vector<ModulePackage *> requiresModuleEnablement(const libdnf::PackageSet & packages);

    /**
    * @brief Enable module stream. Return true if requested change realy triggers a change in
    * the persistor.
    * When the count parameter is set to false the change will not count towards the limit of
    * module state modifications.
    * It can throw ModulePackageContainer::EnableMultipleStreamsException or
    * ModulePackageContainer::NoModuleException exceprion if module do not exist
    *
    * @return bool
    */
    bool enable(const std::string &name, const std::string &stream, const bool count = true);

    /**
    * @brief Enable module stream. Return true if requested changes realy triggers a change in
    * the persistor.
    * When the count parameter is set to false the change will not count towards the limit of
    * module state modifications.
    * It can throw ModulePackageContainer::EnableMultipleStreamsException or
    * ModulePackageContainer::NoModuleException exceprion if module do not exist
    *
    * @return bool
    */
    bool enable(const ModulePackage * module, const bool count = true);
    /**
     * @brief unmark module 'name' from any streams
     * When the count parameter is set to false the change will not count towards the limit of
     * module state modifications.
     */
    void disable(const std::string & name, const bool count = true);
    void disable(const ModulePackage * module, const bool count = true);
    /**
     * @brief Reset module state so it's no longer enabled or disabled.
     * When the count parameter is set to false the change will not count towards the limit of
     * module state modifications.
     */
    void reset(const std::string &name, const bool count = true);
    void reset(const ModulePackage * module, const bool count = true);
    /**
     * @brief add profile to name:stream
     */
    void install(const std::string &name, const std::string &stream, const std::string &profile);
    void install(const ModulePackage * module, const std::string &profile);
    /**
     * @brief remove profile from name:stream
     */
    void uninstall(const std::string &name, const std::string &stream, const std::string &profile);
    void uninstall(const ModulePackage * module, const std::string &profile);
    /**
     * @brief commit module changes to storage
     */
    void save();
    /**
     * @brief discard all module changes and revert to storage state
     */
    void rollback();
    /**
     * @brief Are there any changes to be saved?
     */
    bool isChanged();

    bool isEnabled(const std::string &name, const std::string &stream);
    bool isEnabled(const ModulePackage * module);

    bool isDisabled(const std::string &name);
    bool isDisabled(const ModulePackage * module);
    ModuleState getModuleState(const std::string & name);
    std::set<std::string> getInstalledPkgNames();

    std::string getReport();

    /**
    * @brief Get configured default profiles for module stream
    */
    std::vector<std::string> getDefaultProfiles(std::string moduleName, std::string moduleStream);

    /**
     * @brief Get configured default stream for a module
     */
    const std::string & getDefaultStream(const std::string &name) const;

    /**
     * @brief get enabled stream for a module
     */
    const std::string & getEnabledStream(const std::string &name);

    /**
     * @brief list of name:stream for module streams that are to be enable
     */
    std::map<std::string, std::string> getEnabledStreams();

    /**
     * @brief list of names of modules that are to be disabled
     */
    std::vector<std::string> getDisabledModules();

    /**
     * @brief list of name:stream for module streams that are to be disabled
     * "Will be removed after 2019-12-31. Use getDisabledModules() instead."
     */
    DEPRECATED("Will be removed after 2019-12-31. Use getDisabledModules() instead.")
    std::map<std::string, std::string> getDisabledStreams();

    /**
     * @brief list of names of modules that are to be reset
     */
    std::vector<std::string> getResetModules();

    /**
     * @brief list of name:stream for module streams that are to be reset
     * "Will be removed after 2019-12-31. Use getResetModules() instead."
     */
    DEPRECATED("Will be removed after 2019-12-31. Use getResetModules() instead.")
    std::map<std::string, std::string> getResetStreams();
    /**
     * @brief list of name:<old_stream:new_stream> for modules whose stream has changed
     */
    std::map<std::string, std::pair<std::string, std::string>> getSwitchedStreams();
    /**
     * @brief list of name:[profiles] for module profiles being added
     */
    std::map<std::string, std::vector<std::string>> getInstalledProfiles();

    /**
     * @brief list of installed profiles for module name
     */
    std::vector<std::string> getInstalledProfiles(std::string moduleName);
    /**
     * @brief list of name:[profiles] for module profiles being removed
     */
    std::map<std::string, std::vector<std::string>> getRemovedProfiles();
    /**
    * @brief Query modules according libdnf::Nsvcap. But the search ignores profiles
    *
    */
    std::vector<ModulePackage *> query(libdnf::Nsvcap & moduleNevra);
    /**
    * @brief Requiers subject in format <name>, <name>:<stream>, or <name>:<stream>:<contex>
    *
    * @param subject p_subject:...
    * @return std::vector<ModulePackage *>
    */
    std::vector<ModulePackage *> query(std::string subject);
    std::vector<ModulePackage *> query(std::string name, std::string stream,
        std::string version, std::string context, std::string arch);
    void enableDependencyTree(std::vector<ModulePackage *> & modulePackages);
    std::pair<std::vector<std::vector<std::string>>, ModulePackageContainer::ModuleErrorType> resolveActiveModulePackages(bool debugSolver);
    bool isModuleActive(Id id);
    bool isModuleActive(const ModulePackage * modulePackage);
    void loadFailSafeData();
    void updateFailSafeData();
    void applyObsoletes();

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}

#endif //LIBDNF_MODULEPACKAGECONTAINER_HPP
