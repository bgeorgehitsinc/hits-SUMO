/****************************************************************************/
// Eclipse SUMO, Simulation of Urban MObility; see https://eclipse.org/sumo
// Copyright (C) 2001-2021 German Aerospace Center (DLR) and others.
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// https://www.eclipse.org/legal/epl-2.0/
// This Source Code may also be made available under the following Secondary
// Licenses when the conditions for such availability set forth in the Eclipse
// Public License 2.0 are satisfied: GNU General Public License, version 2
// or later which is available at
// https://www.gnu.org/licenses/old-licenses/gpl-2.0-standalone.html
// SPDX-License-Identifier: EPL-2.0 OR GPL-2.0-or-later
/****************************************************************************/
/// @file    GNEMoveElement.cpp
/// @author  Pablo Alvarez Lopez
/// @date    Mar 2020
///
// Class used for move shape elements
/****************************************************************************/
#include <netedit/elements/network/GNEEdge.h>
#include <netedit/changes/GNEChange_Attribute.h>
#include <netedit/GNEViewNet.h>

#include "GNEMoveElement.h"


// ===========================================================================
// GNEMoveOperation method definitions
// ===========================================================================

GNEMoveOperation::GNEMoveOperation(GNEMoveElement* _moveElement,
                                   const Position _originalPosition) :
    moveElement(_moveElement),
    originalShape({_originalPosition}),
    shapeToMove({_originalPosition}),
    allowChangeLane(false),
    operationType(OperationType::POSITION) {
}


GNEMoveOperation::GNEMoveOperation(GNEMoveElement* _moveElement,
                                   const PositionVector _originalShape) :
    moveElement(_moveElement),
    originalShape(_originalShape),
    shapeToMove(_originalShape),
    allowChangeLane(false),
    operationType(OperationType::SHAPE) {
}


GNEMoveOperation::GNEMoveOperation(GNEMoveElement* _moveElement,
                                   const PositionVector _originalShape,
                                   const std::vector<int> _originalgeometryPoints,
                                   const PositionVector _shapeToMove,
                                   const std::vector<int> _geometryPointsToMove) :
    moveElement(_moveElement),
    originalShape(_originalShape),
    originalGeometryPoints(_originalgeometryPoints),
    shapeToMove(_shapeToMove),
    geometryPointsToMove(_geometryPointsToMove),
    allowChangeLane(false),
    operationType(OperationType::GEOMETRY_POINTS) {
}


GNEMoveOperation::GNEMoveOperation(GNEMoveElement* _moveElement,
                                   const GNELane* _lane,
                                   const double _firstPosition,
                                   const bool _allowChangeLane) :
    moveElement(_moveElement),
    firstLane(_lane),
    firstPosition(_firstPosition * _lane->getLengthGeometryFactor()),
    allowChangeLane(_allowChangeLane),
    operationType(OperationType::ONE_LANE) {
}


GNEMoveOperation::GNEMoveOperation(GNEMoveElement* _moveElement,
                                   const GNELane* _lane,
                                   const double _firstPosition,
                                   const double _secondPosition,
                                   const bool _allowChangeLane,
                                   const OperationType _operationType) :
    moveElement(_moveElement),
    firstLane(_lane),
    firstPosition(_firstPosition * _lane->getLengthGeometryFactor()),
    secondPosition(_secondPosition * _lane->getLengthGeometryFactor()),
    allowChangeLane(_allowChangeLane),
    operationType(_operationType) {
}


GNEMoveOperation::GNEMoveOperation(GNEMoveElement* _moveElement,
                                   const GNELane* _firstLane,
                                   const double _firstStartPos,
                                   const GNELane* _secondLane,
                                   const double _secondStartPos,
                                   const bool _allowChangeLane,
                                   const OperationType _operationType) :
    moveElement(_moveElement),
    firstLane(_firstLane),
    firstPosition((_firstStartPos != INVALID_DOUBLE) ? _firstStartPos * _firstLane->getLengthGeometryFactor() : INVALID_DOUBLE),
    secondLane(_secondLane),
    secondPosition((_secondStartPos != INVALID_DOUBLE) ? _secondStartPos * _secondLane->getLengthGeometryFactor() : INVALID_DOUBLE),
    allowChangeLane(_allowChangeLane),
    operationType(_operationType) {
}


GNEMoveOperation::~GNEMoveOperation() {}

// ===========================================================================
// GNEMoveOffset method definitions
// ===========================================================================

GNEMoveOffset::GNEMoveOffset() :
    x(0),
    y(0),
    z(0) {
}


GNEMoveOffset::GNEMoveOffset(const double x_, const double y_) :
    x(x_),
    y(y_),
    z(0) {
}


GNEMoveOffset::GNEMoveOffset(const double z_) :
    x(0),
    y(0),
    z(z_) {
}


GNEMoveOffset::~GNEMoveOffset() {}

// ===========================================================================
// GNEMoveResult method definitions
// ===========================================================================

GNEMoveResult::GNEMoveResult(const GNEMoveOperation* moveOperation) :
    operationType(moveOperation->operationType),
    firstLaneOffset(0),
    newFirstLane(nullptr),
    newFirstPos(0),
    secondLaneOffset(0),
    newSecondLane(nullptr),
    newSecondPos(0) {}


GNEMoveResult::~GNEMoveResult() {}


void
GNEMoveResult::clearLanes() {
    firstLaneOffset = 0;
    newFirstLane = nullptr;
    secondLaneOffset = 0;
    newSecondLane = nullptr;
}

// ===========================================================================
// GNEMoveElement method definitions
// ===========================================================================

GNEMoveElement::GNEMoveElement() :
    myMoveElementLateralOffset(0) {
}


GNEMoveOperation* 
GNEMoveElement::calculateMoveShapeOperation(const PositionVector originalShape, const Position mousePosition, const double snapRadius, const bool onlyContour) {
    // calculate squared snapRadius
    const double squaredSnapRadius = (snapRadius * snapRadius);
    // declare shape to move
    PositionVector shapeToMove = originalShape;
    // obtain nearest index
    const int nearestIndex = originalShape.indexOfClosest(mousePosition);
    // obtain nearest position
    const Position nearestPosition = originalShape.positionAtOffset2D(originalShape.nearest_offset_to_point2D(mousePosition));
    // check conditions
    if (nearestIndex == -1) {
        return nullptr;
    } else if (nearestPosition == Position::INVALID) {
        // special case for extremes
        if (mousePosition.distanceSquaredTo2D(shapeToMove[nearestIndex]) <= squaredSnapRadius) {
            // move extrem without creating new geometry point
            return new GNEMoveOperation(this, originalShape, {nearestIndex}, shapeToMove, {nearestIndex});
        } else {
            return nullptr;
        }
    } else if (nearestPosition.distanceSquaredTo2D(shapeToMove[nearestIndex]) <= squaredSnapRadius) {
        // move geometry point without creating new geometry point
        return new GNEMoveOperation(this, originalShape, {nearestIndex}, shapeToMove, {nearestIndex});
    } else if (!onlyContour || nearestPosition.distanceSquaredTo2D(mousePosition) <= squaredSnapRadius) {
        // create new geometry point and keep new index (if we clicked near of shape)
        const int newIndex = shapeToMove.insertAtClosest(nearestPosition, true);
        // move after setting new geometry point in shapeToMove
        return new GNEMoveOperation(this, originalShape, {nearestIndex}, shapeToMove, {newIndex});
    } else {
        return nullptr;
    }
}


void
GNEMoveElement::moveElement(const GNEViewNet* viewNet, GNEMoveOperation* moveOperation, const GNEMoveOffset& offset) {
    // declare move result
    GNEMoveResult moveResult(moveOperation);
    // set geometry points to move
    moveResult.geometryPointsToMove = moveOperation->geometryPointsToMove;
    // check if we're moving over a lane shape, an entire shape or only certain geometry point
    if (moveOperation->firstLane) {
        // calculate movement over lane
        if (moveOperation->secondLane) {
            if (moveOperation->operationType == GNEMoveOperation::OperationType::TWO_LANES_MOVEFIRST) {
                // move only first position
                calculateSingleMovementOverOneLane(moveResult, viewNet, moveOperation->firstLane, moveOperation->firstPosition, offset,
                                                   0, moveOperation->firstLane->getLaneShapeLength());
            } else if (moveOperation->operationType == GNEMoveOperation::OperationType::TWO_LANES_MOVESECOND) {
                // move only two position
                calculateSingleMovementOverOneLane(moveResult, viewNet, moveOperation->secondLane, moveOperation->secondPosition, offset,
                                                   0, moveOperation->secondLane->getLaneShapeLength());
            } else {
                // move both first and second positions
                calculateDoubleMovementOverTwoLanes(moveResult, viewNet, moveOperation, offset);
            }
            // calculate new lane
            if (moveOperation->allowChangeLane) {
                calculateNewLane(viewNet, moveOperation->firstLane, moveResult.newFirstLane, moveResult.firstLaneOffset);
                calculateNewLane(viewNet, moveOperation->secondLane, moveResult.newSecondLane, moveResult.secondLaneOffset);
            } else {
                moveResult.clearLanes();
            }
        } else {
            if (moveOperation->operationType == GNEMoveOperation::OperationType::ONE_LANE) {
                // move first position around the entire lane
                calculateSingleMovementOverOneLane(moveResult, viewNet, moveOperation->firstLane, moveOperation->firstPosition, offset,
                                                   0, moveOperation->firstLane->getLaneShapeLength());
            } else if (moveOperation->operationType == GNEMoveOperation::OperationType::ONE_LANE_MOVEFIRST) {
                // move first position around [0, secondPosition]
                calculateSingleMovementOverOneLane(moveResult, viewNet, moveOperation->firstLane, moveOperation->firstPosition, offset,
                                                   0, moveOperation->secondPosition);
            } else if (moveOperation->operationType == GNEMoveOperation::OperationType::ONE_LANE_MOVESECOND) {
                // move first position around [firstPosition, laneLenght]
                calculateSingleMovementOverOneLane(moveResult, viewNet, moveOperation->firstLane, moveOperation->secondPosition, offset,
                                                   moveOperation->firstPosition, moveOperation->firstLane->getLaneShapeLength());
            } else {
                // move both first and second positions
                calculateDoubleMovementOverOneLane(moveResult, viewNet, moveOperation->firstLane, moveOperation->firstPosition, 
                                                   moveOperation->secondPosition, offset);
            }
            // calculate new lane
            if (moveOperation->allowChangeLane) {
                calculateNewLane(viewNet, moveOperation->firstLane, moveResult.newFirstLane, moveResult.firstLaneOffset);
            } else {
                moveResult.clearLanes();
            }
        }
    } else if (moveOperation->geometryPointsToMove.empty()) {
        // set values in moveResult
        moveResult.shapeToUpdate = moveOperation->shapeToMove;
        // move entire shape
        for (auto& geometryPointIndex : moveResult.shapeToUpdate) {
            if (geometryPointIndex != Position::INVALID) {
                // add offset
                geometryPointIndex.add(offset.x, offset.y, offset.z);
                // apply snap to active grid
                geometryPointIndex = viewNet->snapToActiveGrid(geometryPointIndex);
            } else {
                throw ProcessError("trying to move an invalid position");
            }
        }
    } else {
        // set values in moveResult
        moveResult.shapeToUpdate = moveOperation->shapeToMove;
        // move geometry points
        for (const auto& geometryPointIndex : moveOperation->geometryPointsToMove) {
            if (moveResult.shapeToUpdate[geometryPointIndex] != Position::INVALID) {
                // add offset
                moveResult.shapeToUpdate[geometryPointIndex].add(offset.x, offset.y, offset.z);
                // apply snap to active grid
                moveResult.shapeToUpdate[geometryPointIndex] = viewNet->snapToActiveGrid(moveResult.shapeToUpdate[geometryPointIndex]);
            } else {
                throw ProcessError("trying to move an invalid position");
            }
        }
    }
    // move shape element
    moveOperation->moveElement->setMoveShape(moveResult);
}


void
GNEMoveElement::commitMove(const GNEViewNet* viewNet, GNEMoveOperation* moveOperation, const GNEMoveOffset& offset, GNEUndoList* undoList) {
    // declare move result
    GNEMoveResult moveResult(moveOperation);
    // check if we're moving over a lane shape, an entire shape or only certain geometry point
    if (moveOperation->firstLane) {
        // calculate original move result
        moveResult.newFirstLane = moveOperation->firstLane;
        moveResult.newFirstPos = moveOperation->firstPosition;
        moveResult.newSecondLane = moveOperation->secondLane;
        moveResult.newSecondPos = moveOperation->secondPosition;
        // set original positions in element
        moveOperation->moveElement->setMoveShape(moveResult);
        // calculate movement over lane
        if (moveOperation->secondLane) {
            calculateDoubleMovementOverTwoLanes(moveResult, viewNet, moveOperation, offset);
            // calculate new lane
            if (moveOperation->allowChangeLane) {
                calculateNewLane(viewNet, moveOperation->firstLane, moveResult.newFirstLane, moveResult.firstLaneOffset);
                calculateNewLane(viewNet, moveOperation->secondLane, moveResult.newSecondLane, moveResult.secondLaneOffset);
            } else {
                moveResult.clearLanes();
            }
        } else {
            if (moveOperation->operationType == GNEMoveOperation::OperationType::ONE_LANE) {
                // move first position around the entire lane
                calculateSingleMovementOverOneLane(moveResult, viewNet, moveOperation->firstLane, moveOperation->firstPosition, offset,
                                                   0, moveOperation->firstLane->getLaneShapeLength());
            } else if (moveOperation->operationType == GNEMoveOperation::OperationType::ONE_LANE_MOVEFIRST) {
                // move first position around [0, secondPosition]
                calculateSingleMovementOverOneLane(moveResult, viewNet, moveOperation->firstLane, moveOperation->firstPosition, offset,
                                                   0, moveOperation->secondPosition);
                } else if (moveOperation->operationType == GNEMoveOperation::OperationType::ONE_LANE_MOVESECOND) {
                // move first position around [firstPosition, laneLenght]
                calculateSingleMovementOverOneLane(moveResult, viewNet, moveOperation->firstLane, moveOperation->secondPosition, offset,
                                                   moveOperation->firstPosition, moveOperation->firstLane->getLaneShapeLength());
            } else {
                // move both first and second positions
                calculateDoubleMovementOverOneLane(moveResult, viewNet, moveOperation->firstLane, moveOperation->firstPosition, 
                                                   moveOperation->secondPosition, offset);
            }
            // calculate new lane
            if (moveOperation->allowChangeLane) {
                calculateNewLane(viewNet, moveOperation->firstLane, moveResult.newFirstLane, moveResult.firstLaneOffset);
            } else {
                moveResult.clearLanes();
            }
            // calculate new lane
            if (moveOperation->allowChangeLane) {
                calculateNewLane(viewNet, moveOperation->firstLane, moveResult.newFirstLane, moveResult.firstLaneOffset);
            } else {
                moveResult.clearLanes();
            }
        }
    } else {
        // set original geometry points to move
        moveResult.geometryPointsToMove = moveOperation->originalGeometryPoints;
        // set shapeToUpdate with originalPosOverLanes
        moveResult.shapeToUpdate = moveOperation->originalShape;
        // first restore original geometry geometry
        moveOperation->moveElement->setMoveShape(moveResult);
        // set new geometry points to move
        moveResult.geometryPointsToMove = moveOperation->geometryPointsToMove;
        // set values in moveResult
        moveResult.shapeToUpdate = moveOperation->shapeToMove;
        // check if we're moving an entire shape or  only certain geometry point
        if (moveOperation->geometryPointsToMove.empty()) {
            // move entire shape
            for (auto& geometryPointIndex : moveResult.shapeToUpdate) {
                if (geometryPointIndex != Position::INVALID) {
                    // add offset
                    geometryPointIndex.add(offset.x, offset.y, offset.z);
                    // apply snap to active grid
                    geometryPointIndex = viewNet->snapToActiveGrid(geometryPointIndex);
                } else {
                    throw ProcessError("trying to move an invalid position");
                }
            }
        } else {
            // only move certain geometry points
            for (const auto& geometryPointIndex : moveOperation->geometryPointsToMove) {
                if (moveResult.shapeToUpdate[geometryPointIndex] != Position::INVALID) {
                    // add offset
                    moveResult.shapeToUpdate[geometryPointIndex].add(offset.x, offset.y, offset.z);
                    // apply snap to active grid
                    moveResult.shapeToUpdate[geometryPointIndex] = viewNet->snapToActiveGrid(moveResult.shapeToUpdate[geometryPointIndex]);
                } else {
                    throw ProcessError("trying to move an invalid position");
                }
            }
            // remove double points (only in commitMove)
            if (moveResult.shapeToUpdate.size() > 2) {
                moveResult.shapeToUpdate.removeDoublePoints(2);
            }
        }
    }
    // commit move shape
    moveOperation->moveElement->commitMoveShape(moveResult, undoList);
}


 double 
 GNEMoveElement::calculateLaneOffset(const GNEViewNet* viewNet, const GNELane *lane, const double firstPosition, const double secondPosition, 
                                     const GNEMoveOffset &offset, const double extremFrom, const double extremTo) {
    // declare laneOffset
    double laneOffset = 0;
    // calculate central position between two given positions
    const double offsetCentralPosition = (firstPosition + secondPosition) * 0.5;
    // calculate middle lenght between two given positions
    const double middleLenght = std::abs(secondPosition - firstPosition) * 0.5;
    // calculate lane position at offset given by offsetCentralPosition
    Position laneCentralPosition = lane->getLaneShape().positionAtOffset2D(offsetCentralPosition);
    // apply offset to positionAtCentralPosition
    laneCentralPosition.add(offset.x, offset.y, offset.z);
    // snap to grid
    laneCentralPosition = viewNet->snapToActiveGrid(laneCentralPosition);
    // calculate offset over lane using laneCentralPosition
    const double offsetLaneCentralPositionPerpendicular = lane->getLaneShape().nearest_offset_to_point2D(laneCentralPosition);
    // check if offset is within lane shape
    if (offsetLaneCentralPositionPerpendicular == -1) {
        // calculate non-perpendicular offset over lane using laneCentralPosition
        const double offsetLaneCentralPosition = lane->getLaneShape().nearest_offset_to_point2D(laneCentralPosition, false);
        // due laneCentralPosition is out of lane shape, then place positions in extremes
        if (offsetLaneCentralPosition == 0) {
            laneOffset = firstPosition;
        } else {
            laneOffset = secondPosition - lane->getLaneShape().length2D();
        }
    } else {
        // laneCentralPosition is within of lane shapen, then calculate offset using middlelenght
        if ((offsetLaneCentralPositionPerpendicular - middleLenght) < extremFrom) {
            laneOffset = firstPosition + extremFrom;
        } else if ((offsetLaneCentralPositionPerpendicular + middleLenght) > extremTo) {
            laneOffset = secondPosition - extremTo;
        } else {
            laneOffset = (offsetCentralPosition - offsetLaneCentralPositionPerpendicular);
        }
    }
    return laneOffset;
 }


void
GNEMoveElement::calculateSingleMovementOverOneLane(GNEMoveResult& moveResult, const GNEViewNet* viewNet, const GNELane* lane, 
        const double pos, const GNEMoveOffset& offset, const double extremFrom, const double extremTo) {
    // get lane offset
    const double laneOffset = calculateLaneOffset(viewNet, lane, pos, pos, offset, extremFrom, extremTo);
    // update moveResult
    moveResult.newFirstPos = (pos - laneOffset) / lane->getLengthGeometryFactor();
    moveResult.newSecondPos = 0;
}


void
GNEMoveElement::calculateDoubleMovementOverOneLane(GNEMoveResult& moveResult, const GNEViewNet* viewNet, const GNELane* lane, 
        const double firstPos, const double secondPos, const GNEMoveOffset& offset) {
    // get lane offset
    const double laneOffset = calculateLaneOffset(viewNet, lane, firstPos, secondPos, offset, 0, lane->getLaneShape().length2D());
    // update moveResult
    moveResult.newFirstPos = (firstPos - laneOffset) / lane->getLengthGeometryFactor();
    moveResult.newSecondPos = (secondPos - laneOffset) / lane->getLengthGeometryFactor();
}


void
GNEMoveElement::calculateDoubleMovementOverTwoLanes(GNEMoveResult& moveResult, const GNEViewNet* viewNet, const GNEMoveOperation* moveOperation, 
        const GNEMoveOffset& offset) {
    // calculate lenght between pos over lanes
    const double centralPosition = (moveOperation->firstPosition + moveOperation->secondPosition) * 0.5;
    // calculate middle lenght between first and last pos over lanes
    const double middleLenght = std::abs(moveOperation->secondPosition - moveOperation->firstPosition) * 0.5;
    // get lane length
    const double laneShapeLengt = moveOperation->firstLane->getLaneShape().length2D();
    // declare position over lane offset
    double posOverLaneOffset = 0;
    // calculate position at offset given by centralPosition
    Position lanePositionAtCentralPosition = moveOperation->firstLane->getLaneShape().positionAtOffset2D(centralPosition);
    // apply offset to positionAtCentralPosition
    lanePositionAtCentralPosition.add(offset.x, offset.y, offset.z);
    // snap to grid
    lanePositionAtCentralPosition = viewNet->snapToActiveGrid(lanePositionAtCentralPosition);
    // calculate new posOverLane perpendicular
    const double newPosOverLanePerpendicular = moveOperation->firstLane->getLaneShape().nearest_offset_to_point2D(lanePositionAtCentralPosition);
    // calculate posOverLaneOffset
    if (newPosOverLanePerpendicular == -1) {
        // calculate new posOverLane non-perpendicular
        const double newPosOverLane = moveOperation->firstLane->getLaneShape().nearest_offset_to_point2D(lanePositionAtCentralPosition, false);
        // out of lane shape, then place element in lane extremes
        if (newPosOverLane == 0) {
            posOverLaneOffset = moveOperation->firstPosition;
        } else {
            posOverLaneOffset = moveOperation->secondPosition - laneShapeLengt;
        }
    } else {
        // within of lane shape
        if ((newPosOverLanePerpendicular - middleLenght) < 0) {
            posOverLaneOffset = moveOperation->firstPosition;
        } else if ((newPosOverLanePerpendicular + middleLenght) > laneShapeLengt) {
            posOverLaneOffset = moveOperation->secondPosition - laneShapeLengt;
        } else {
            posOverLaneOffset = centralPosition - newPosOverLanePerpendicular;
        }
    }
    // update moveResult
    moveResult.newFirstPos = (moveOperation->firstPosition - posOverLaneOffset) / moveOperation->firstLane->getLengthGeometryFactor();
    moveResult.newSecondPos = (moveOperation->secondPosition + posOverLaneOffset) / moveOperation->firstLane->getLengthGeometryFactor();


/*
    // calculate movements over both lanes separately
    if (moveOperation->firstPosition != INVALID_DOUBLE) {
        GNEMoveResult moveResultTemporal(moveOperation);
        calculateSingleMovementOverOneLane(moveResultTemporal, viewNet, moveOperation->firstLane, moveOperation->firstPosition, offset,
                                           0, moveOperation->firstLane->getLaneShapeLength());
        moveResult.newFirstPos = moveResultTemporal.newFirstPos;
    } else {
        moveResult.newFirstPos = INVALID_DOUBLE;
    }
    if (moveOperation->secondPosition != INVALID_DOUBLE) {
        GNEMoveResult moveResultTemporal(moveOperation);
        calculateSingleMovementOverOneLane(moveResultTemporal, viewNet, moveOperation->secondLane, moveOperation->secondPosition, offset,
                                           0, moveOperation->secondLane->getLaneShapeLength());
        moveResult.newSecondPos = moveResultTemporal.newFirstPos;
    } else {
        moveResult.newSecondPos = INVALID_DOUBLE;
    }
*/
}


void
GNEMoveElement::calculateNewLane(const GNEViewNet* viewNet, const GNELane* originalLane, const GNELane*& newLane, double& laneOffset) {
    // get cursor position
    const Position cursorPosition = viewNet->getPositionInformation();
    // iterate over edge lanes
    for (const auto& lane : originalLane->getParentEdge()->getLanes()) {
        // avoid moveOperation lane
        if (lane != originalLane) {
            // calculate offset over lane shape
            const double offSet = lane->getLaneShape().nearest_offset_to_point2D(cursorPosition, true);
            // calculate position over lane shape
            const Position posOverLane = lane->getLaneShape().positionAtOffset2D(offSet);
            // check distance
            if (posOverLane.distanceSquaredTo2D(cursorPosition) < 1) {
                // update newlane
                newLane = lane;
                // calculate offset over moveOperation lane
                const double offsetMoveOperationLane = originalLane->getLaneShape().nearest_offset_to_point2D(cursorPosition, true);
                // calculate position over moveOperation lane
                const Position posOverMoveOperationLane = originalLane->getLaneShape().positionAtOffset2D(offsetMoveOperationLane);
                // update moveResult of laneOffset
                laneOffset = posOverLane.distanceTo2D(posOverMoveOperationLane);
                // change sign of  moveResult laneOffset depending of lane index
                if (originalLane->getIndex() < newLane->getIndex()) {
                    laneOffset *= -1;
                }
            }
        }
    }
}

/****************************************************************************/
