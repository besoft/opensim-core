/* -------------------------------------------------------------------------- *
 * OpenSim Moco: MocoControlGoal.cpp                                          *
 * -------------------------------------------------------------------------- *
 * Copyright (c) 2017 Stanford University and the Authors                     *
 *                                                                            *
 * Author(s): Christopher Dembia                                              *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may    *
 * not use this file except in compliance with the License. You may obtain a  *
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0          *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 * -------------------------------------------------------------------------- */

#include "MocoControlGoal.h"

#include "../MocoUtilities.h"

#include <OpenSim/Simulation/Model/Model.h>

using namespace OpenSim;

MocoControlGoal::MocoControlGoal() { constructProperties(); }

void MocoControlGoal::constructProperties() {
    constructProperty_control_weights(MocoWeightSet());
    constructProperty_exponent(2);
    constructProperty_divide_by_displacement(false);
}

void MocoControlGoal::setWeightForControl(
        const std::string& controlName, const double& weight) {
    if (get_control_weights().contains(controlName)) {
        upd_control_weights().get(controlName).setWeight(weight);
    } else {
        upd_control_weights().cloneAndAppend({controlName, weight});
    }
}

void MocoControlGoal::initializeOnModelImpl(const Model& model) const {

    // Get all expected control names.
    auto controlNames = createControlNamesFromModel(model);

    // Check that the model controls are in the correct order.
    checkOrderSystemControls(model);

    auto systemControlIndexMap = createSystemControlIndexMap(model);
    // Make sure there are no weights for nonexistent controls.
    for (int i = 0; i < get_control_weights().getSize(); ++i) {
        const auto& thisName = get_control_weights()[i].getName();
        if (std::find(controlNames.begin(), controlNames.end(), thisName) ==
                controlNames.end()) {
            OPENSIM_THROW_FRMOBJ(
                    Exception, "Unrecognized control '" + thisName + "'.");
        }
    }

    for (const auto& controlName : controlNames) {
        double weight = 1.0;
        if (get_control_weights().contains(controlName)) {
            weight = get_control_weights().get(controlName).getWeight();
        }

        if (weight != 0.0) {
            m_controlIndices.push_back(systemControlIndexMap[controlName]);
            m_weights.push_back(weight);
            m_controlNames.push_back(controlName);
        }
    }

    OPENSIM_THROW_IF_FRMOBJ(get_exponent() < 2, Exception,
            "Exponent must be 2 or greater.");
    m_exponent = get_exponent();

    setNumIntegralsAndOutputs(1, 1);
}

void MocoControlGoal::calcIntegrandImpl(
        const SimTK::State& state, double& integrand) const {
    getModel().realizeVelocity(state); // TODO would avoid this, ideally.
    const auto& controls = getModel().getControls(state);
    integrand = 0;
    int iweight = 0;
    for (const auto& icontrol : m_controlIndices) {
        // TODO: On the first problem in exampleMocoTrack, this more general
        // form causes the problem to take 2 minutes instead of 1 minute 30
        // seconds. So there is a large performance penalty.
        integrand +=
                m_weights[iweight] *
                        pow(std::abs(controls[icontrol]), m_exponent);
        ++iweight;
    }
}

void MocoControlGoal::calcGoalImpl(
        const GoalInput& input, SimTK::Vector& cost) const {
    cost[0] = input.integral;
    if (get_divide_by_displacement()) {
        const SimTK::Vec3 comInitial =
                getModel().calcMassCenterPosition(input.initial_state);
        const SimTK::Vec3 comFinal =
                getModel().calcMassCenterPosition(input.final_state);
        // TODO: Use distance squared for convexity.
        const SimTK::Real displacement = (comFinal - comInitial).norm();
        cost[0] /= displacement;
    }
}

void MocoControlGoal::printDescriptionImpl(std::ostream& stream) const {
    for (int i = 0; i < (int) m_controlNames.size(); i++) {
        stream << "        ";
        stream << "control: " << m_controlNames[i]
               << ", weight: " << m_weights[i] << std::endl;
    }
}
