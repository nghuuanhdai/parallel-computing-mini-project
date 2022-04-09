//================================================================================
// Name        : JobOperatorConfiguratorTemplate.h
// Author      : Alessio Netti
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Template that implements a configurator for Job operator plugins.
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2018-2019 Leibniz Supercomputing Centre
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//================================================================================

#ifndef PROJECT_JOBOPERATORCONFIGURATORTEMPLATE_H
#define PROJECT_JOBOPERATORCONFIGURATORTEMPLATE_H

#include "OperatorConfiguratorTemplate.h"
#include "JobOperatorTemplate.h"

/**
 * @brief Template that implements a configurator for Job operator plugins.
 *
 * @details This template expands the standard OperatorConfiguratorTemplate,
 *          with very few changes to accomodate the different design of job
 *          operators.
 *
 * @ingroup operator
 */
template <class Operator, class SBase = SensorBase>
class JobOperatorConfiguratorTemplate : virtual public OperatorConfiguratorTemplate<Operator, SBase> {
    
    // Verifying the types of input classes
    static_assert(std::is_base_of<SensorBase, SBase>::value, "SBase must derive from SensorBase!");
    static_assert(std::is_base_of<OperatorInterface, Operator>::value, "Operator must derive from OperatorInterface!");

protected:

    // For readability
    using O_Ptr = std::shared_ptr<Operator>;

public:

    /**
    * @brief            Class constructor
    */
    JobOperatorConfiguratorTemplate() : OperatorConfiguratorTemplate<Operator, SBase>() {}

    /**
    * @brief            Copy constructor is not available
    */
    JobOperatorConfiguratorTemplate(const JobOperatorConfiguratorTemplate&) = delete;

    /**
    * @brief            Assignment operator is not available
    */
    JobOperatorConfiguratorTemplate& operator=(const JobOperatorConfiguratorTemplate&) = delete;

    /**
    * @brief            Class destructor
    */
    virtual ~JobOperatorConfiguratorTemplate() {}

protected:

    /**
     * @brief               Instantiates all necessary units for a single (job) operator
     * 
     *                      When using job operators, the only unit that is always instantiated is ALWAYS the template
     *                      unit, similarly to ordinary operators in on-demand mode. Such unit then is used at runtime,
     *                      even in streaming mode, to build dynamically all appropriate units for jobs that are 
     *                      currently running in the system.
     * 
     * @param op                  The operator whose units must be created
     * @param protoInputs         The vector of prototype input sensors
     * @param protoOutputs        The vector of prototype output sensors
     * @param protoGlobalOutputs  The vector of prototype global output sensors, if any
     * @param inputMode           Input mode to be used (selective, all or all_recursive)
     * @return                    True if successful, false otherwise
     */
    virtual bool readUnits(Operator& op, std::vector<shared_ptr<SBase>>& protoInputs, std::vector<shared_ptr<SBase>>& protoOutputs,
            std::vector<shared_ptr<SBase>>& protoGlobalOutputs, inputMode_t inputMode) {
        // Forcing the job operator to not be duplicated
        if(op.getDuplicate()) {
            LOG(warning) << this->_operatorName << " " << op.getName() << ": The units of this operator cannot be duplicated.";
            op.setDuplicate(false);
        }
        shared_ptr<UnitTemplate<SBase>> jobUnit;
        try {
            jobUnit = this->_unitGen.generateHierarchicalUnit(SensorNavigator::rootKey, std::list<std::string>(), protoGlobalOutputs,
                    protoInputs, protoOutputs, inputMode, op.getMqttPart(), true, op.getEnforceTopics(), op.getRelaxed());
        }
        catch (const std::exception &e) {
            LOG(error) << this->_operatorName << " " << op.getName() << ": Error when creating template job unit: " << e.what();
            return false;
        }
        
        op.clearUnits();
        if (this->unit(*jobUnit)) {
            op.addToUnitCache(jobUnit);
            LOG(debug) << "    Template job unit " + jobUnit->getName() + " generated.";
        } else {
            LOG(error) << "    Template job unit " << jobUnit->getName() << " did not pass the final check!";
            return false;
        }
        return true;
    }

    // Logger object
    boost::log::sources::severity_logger<boost::log::trivial::severity_level> lg;
};

#endif //PROJECT_JOBOPERATORCONFIGURATORTEMPLATE_H
