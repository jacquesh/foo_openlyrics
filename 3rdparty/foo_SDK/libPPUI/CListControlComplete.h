#pragma once

// ================================================================================
// CListControlComplete
// ================================================================================
// Simplified declaration of the base class that most CListControl users will need.
// The other base classes are used directly mainly by old code predating libPPUI.
// ================================================================================

#include "CListControlWithSelection.h"
#include "CListControl_EditImpl.h"
#include "CListAccessible.h"

// ================================================================================
// CListControlWithSelectionImpl = list control with selection/focus
// CListControl_EditImpl = inplace editbox implementation
// CListControlAccImpl = accessibility API implementation (screen reader interop)
// ================================================================================
typedef CListControlAccImpl<CListControl_EditImpl<CListControlWithSelectionImpl> > CListControlComplete;

// CListControlReadOnly : no inplace edit functionality (CListControl_EditImpl)
typedef CListControlAccImpl<CListControlWithSelectionImpl> CListControlReadOnly;
