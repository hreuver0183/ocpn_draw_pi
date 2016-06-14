/***************************************************************************
 *
 * Project:  OpenCPN
 *
 ***************************************************************************
 *   Copyright (C) 2013 by David S. Register                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.         *
 **************************************************************************/

#include "GZ.h"
#include "GZProp.h"
#include "ODdc.h"
#include "ocpn_draw_pi.h"
#include "ODPoint.h"
#include "ODSelect.h"
#include "PathMan.h"
#include "ODUtils.h"
#include "cutil.h"
#include "clipper.hpp"

#ifdef __WXMSW__
#include "GL/gl.h"            // local copy for Windows
#include <GL/glu.h>
#else

#ifndef __OCPN__ANDROID__
#include <GL/gl.h>
#include <GL/glu.h>
#else
#include "qopengl.h"                  // this gives us the qt runtime gles2.h
#include "GL/gl_private.h"
#endif

#endif

#ifdef ocpnUSE_GL
#include <wx/glcanvas.h>
#endif


#include <wx/graphics.h>

#include <wx/listimpl.cpp>

using namespace std;
using namespace ClipperLib;

WX_DEFINE_LIST ( GZList );
extern wxColour     g_colourActiveBoundaryLineColour;
extern wxColour     g_colourInActiveBoundaryLineColour;
extern wxColour     g_colourActiveBoundaryFillColour;
extern wxColour     g_colourInActiveBoundaryFillColour;


extern wxColour     g_colourActiveGZLineColour;
extern wxColour     g_colourInActiveGZLineColour;
extern wxColour     g_colourActiveGZFillColour;
extern wxColour     g_colourInActiveGZFillColour;
extern bool         g_bExclusionGZ;
extern bool         g_bInclusionGZ;
extern ocpn_draw_pi *g_ocpn_draw_pi;
extern unsigned int g_uiGZFillTransparency;
extern int          g_iInclusionGZSize;
extern bool         g_bGZRotateWithBoat;
extern int          g_iGZMaintainWith;
extern ODPlugIn_Position_Fix_Ex  g_pfFix;
extern ODSelect     *g_pODSelect;
extern PathMan      *g_pPathMan;
extern ODConfig     *g_pODConfig;
extern GZProp       *g_pGZPropDialog;
extern int          g_iGZPersistenceType;
extern int          g_iGZMaxNum;

extern int g_path_line_width;

GZ::GZ() : ODPath()
{
    m_sTypeString = _T("GuardZone");

    m_wxcActiveLineColour = g_colourActiveGZLineColour;
    m_wxcActiveFillColour = g_colourActiveGZFillColour;
    m_wxcInActiveLineColour = g_colourInActiveGZLineColour;
    m_wxcInActiveFillColour = g_colourInActiveGZFillColour;
    m_uiFillTransparency = g_uiGZFillTransparency;
    m_dFirstDistance = 0;
    m_dFirstLineDirection = 0;
    m_dSecondDistance = 0;
    m_dSecondLineDirection = 0;
    m_bRotateWithBoat = g_bGZRotateWithBoat;
    m_iMaintainWith = g_iGZMaintainWith;
    m_bSetTransparent = false;
    m_iPersistenceType = g_iGZPersistenceType;
    SetActiveColours();
    
}

GZ::~GZ()
{
    //dtor
}

void GZ::Draw( ODDC& dc, PlugIn_ViewPort &piVP )
{
    if(m_dSecondDistance == 0) return;
    
    m_bSetTransparent = true;
    ODPath::Draw( dc, piVP );
    m_bSetTransparent = false;
    SetActiveColours();
    
    wxPoint l_l1p1;
    wxPoint l_l1p2;
    wxPoint l_l2p1;
    wxPoint l_l2p2;
    wxPoint l_pCentre;
    
    GetLatLonPoints( piVP, &l_pCentre, &l_l1p1, &l_l1p2, &l_l2p1, &l_l2p2 );

    wxColour tCol;
    tCol.Set(m_fillcol.Red(), m_fillcol.Green(), m_fillcol.Blue(), m_uiFillTransparency);
    dc.SetPen( *wxThePenList->FindOrCreatePen( m_col, m_width, m_style ) );
    dc.SetBrush( *wxTheBrushList->FindOrCreateBrush( tCol, wxBRUSHSTYLE_CROSSDIAG_HATCH ) );

    RenderArcSegment( dc, &l_pCentre, &l_l1p1, &l_l1p2, &l_l2p2, &l_l2p1, piVP, false );
    
}

void GZ::DrawGL( PlugIn_ViewPort &piVP )
{
#ifdef ocpnUSE_GL
    if(m_dSecondDistance == 0) return;
    if ( !m_bVisible ) return;
    
    wxPoint l_l1p1;
    wxPoint l_l1p2;
    wxPoint l_l2p1;
    wxPoint l_l2p2;
    wxPoint l_pCentre;
    
    GetLatLonPoints( piVP, &l_pCentre, &l_l1p1, &l_l1p2, &l_l2p1, &l_l2p2 );
    ODDC dc;
    
    m_bSetTransparent = true;
    ODPath::DrawGL( piVP );
    m_bSetTransparent = false;
    
    int style = wxPENSTYLE_SOLID;
    int width = g_path_line_width;
    
    if( m_style != STYLE_UNDEFINED ) style = m_style;
    if( m_width != STYLE_UNDEFINED ) width = m_width;
    
    SetActiveColours();
    
    
    // Each byte represents a single pixel for Alpha. This provides a cross hatch in a 16x16 pixel square
    GLubyte slope_cross_hatch[] = {
        0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
        0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00,
        0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x00,
        0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00,
        0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 
        0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF,
        0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00,
        0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x00,
        0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xFF, 0x00,
        0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF
    };        
    
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture( GL_TEXTURE_2D, textureID );
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_ALPHA, 16, 16, 0, GL_ALPHA, GL_UNSIGNED_BYTE, slope_cross_hatch );
    dc.SetTextureSize( 16, 16 );
    glEnable( GL_TEXTURE_2D );
    glEnable( GL_BLEND );
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
    wxColour tCol;
    tCol.Set(m_fillcol.Red(), m_fillcol.Green(), m_fillcol.Blue(), m_uiFillTransparency);
    dc.SetBrush( *wxTheBrushList->FindOrCreateBrush( tCol, wxPENSTYLE_SOLID ) );
    
    RenderArcSegment( dc, &l_pCentre, &l_l1p1, &l_l1p2, &l_l2p2, &l_l2p1, piVP, false );
    
    glDisable( GL_LINE_STIPPLE );
    glDisable( GL_BLEND );
    glDisable( GL_TEXTURE_2D );
    glDeleteTextures(1, &textureID);
    
    dc.SetPen( *wxThePenList->FindOrCreatePen( m_col, width, style ) );
    wxPoint *points;
    int numpoints = ArcSectorPoints( *&points, l_pCentre.x, l_pCentre.y, l_l1p1.x, l_l1p1.y, l_l1p2.x, l_l1p2.y, l_l2p2.x, l_l2p2.y, l_l2p1.x, l_l2p1.y, true);
    dc.DrawLines( numpoints, points );
#ifdef __WXOSX__
    delete [] points;
#else
    wxDELETE( points );
#endif
    
#endif
    
}

void GZ::SetActiveColours( void )
{
    ODPath::SetActiveColours();
    
    if(m_bSetTransparent) 
        m_col = wxTransparentColour;
    
    if( m_bVisible && m_bPathIsActive ) m_fillcol = m_wxcActiveFillColour;
    else m_fillcol = m_wxcInActiveFillColour;
}

void GZ::MoveAllPoints( double inc_lat, double inc_lon )
{
    wxODPointListNode *node = m_pODPointList->GetFirst();
    int nodeCount = m_pODPointList->GetCount();
    int i = 1;
    while(node) {
        if(i == nodeCount) break;
        ODPoint *bp = (ODPoint *)node->GetData();
        bp->m_lat -= inc_lat;
        bp->m_lon -= inc_lon;
        node = node->GetNext();
        i++;
    }
}

void GZ::CentreOnBoat( void )
{
    if(m_pODPointList->size() < 2) return;
    ODPoint *pFirstPoint = m_pODPointList->GetFirst()->GetData();
    ODPoint *pLastPoint = m_pODPointList->GetLast()->GetData();
    m_dCentreLat = g_pfFix.Lat;
    m_dCentreLon = g_pfFix.Lon;
    m_dBoatHeading = g_pfFix.Hdt;
    m_dBoatCOG = g_pfFix.Cog;
    
    if(m_bRotateWithBoat) {
        MaintainWith();
    } else {
        PositionBearingDistanceMercator_Plugin(m_dCentreLat, m_dCentreLon, m_dFirstLineDirection, m_dFirstDistance, &pFirstPoint->m_lat, &pFirstPoint->m_lon);
        PositionBearingDistanceMercator_Plugin(m_dCentreLat, m_dCentreLon, m_dSecondLineDirection, m_dSecondDistance, &pLastPoint->m_lat, &pLastPoint->m_lon);
    } 
    
    UpdateGZSelectablePath();
    FinalizeForRendering();

    bool l_bRequestRefresh = true;

    if(g_pGZPropDialog && g_pGZPropDialog->IsShown())
        l_bRequestRefresh = g_pGZPropDialog->UpdateProperties();
    
    m_bSaveUpdates = false;
    if(l_bRequestRefresh)
        RequestRefresh( g_ocpn_draw_pi->m_parent_window );
    
    return;
}

void GZ::UpdateGZ( ODPoint *pGZPoint, bool bUpdateSelectablePath )
{
    if(pGZPoint == m_pODPointList->GetFirst()->GetData()) {
        DistanceBearingMercator_Plugin( pGZPoint->m_lat, pGZPoint->m_lon, g_pfFix.Lat, g_pfFix.Lon, &m_dFirstLineDirection, &m_dFirstDistance );
    } else {
        DistanceBearingMercator_Plugin( pGZPoint->m_lat, pGZPoint->m_lon, g_pfFix.Lat, g_pfFix.Lon, &m_dSecondLineDirection, &m_dSecondDistance );
    }
    if(bUpdateSelectablePath) UpdateGZSelectablePath();

    FinalizeForRendering();
    
    bool prev_bskip = g_pODConfig->m_bSkipChangeSetUpdate;
    g_pODConfig->m_bSkipChangeSetUpdate = false;
    g_pODConfig->UpdatePath( this ); 
    g_pODConfig->m_bSkipChangeSetUpdate = prev_bskip;
    
    for( unsigned int ip = 0; ip < m_pODPointList->GetCount(); ip++ ) {
        ODPath *pp = (ODPath *) m_pODPointList->Item( ip );
        if( g_pPathMan->IsPathValid(pp) ) {
            pp->FinalizeForRendering();
            pp->m_bIsBeingEdited = false;
            
            g_pODConfig->UpdatePath( pp );
            
            pp->SetHiLite( 0 );
        }
    }
    
    //    Update the PathProperties Dialog, if currently shown
    if( ( NULL != g_pGZPropDialog ) && ( g_pGZPropDialog->IsShown() ) ) {
        if( m_pODPointList ) {
            for( unsigned int ip = 0; ip < m_pODPointList->GetCount(); ip++ ) {
                ODPath *pp = (ODPath *) m_pODPointList->Item( ip );
                if( g_pPathMan->IsPathValid(pp) ) {
                    g_pGZPropDialog->SetPathAndUpdate( pp, true );
                }
            }
        }
    }
}

void GZ::UpdateGZSelectablePath( void )
{
    g_pODSelect->DeleteAllSelectablePathSegments( this );
    g_pODSelect->DeleteAllSelectableODPoints( this );

    double  l_dLat, l_dLon;
    double  l_dPrevLat, l_dPrevLon;
    int l_iSteps;
    double l_dStepSize;
    if(m_dFirstLineDirection < m_dSecondLineDirection) {
        l_iSteps = ceil(24 * (fabs(m_dFirstLineDirection-m_dSecondLineDirection))/360);
        l_dStepSize = fabs(m_dFirstLineDirection-m_dSecondLineDirection) / l_iSteps;
    }
    else {
        l_iSteps = 24 - ceil(24 * (fabs((360 - m_dFirstLineDirection) + m_dSecondLineDirection))/360) - 1;
        l_dStepSize = fabs((360 - m_dFirstLineDirection) + m_dSecondLineDirection) / l_iSteps;
    }
    double firstDirection;
    double secondDirection;
    if(m_bRotateWithBoat) {
        switch(m_iMaintainWith) {
            case ID_MAINTAIN_WITH_HEADING:
                if(!isnan(g_pfFix.Hdt)) {
                    firstDirection = g_pfFix.Hdt + m_dFirstLineDirection;
                    secondDirection = g_pfFix.Hdt + m_dSecondLineDirection;
                } else {
                    firstDirection = m_dFirstLineDirection;
                    secondDirection = m_dSecondLineDirection;
                }
                break;
            case ID_MAINTAIN_WITH_COG:
                if(!isnan(g_pfFix.Cog)) {
                    firstDirection = g_pfFix.Cog + m_dFirstLineDirection;
                    secondDirection = g_pfFix.Cog + m_dSecondLineDirection;
                } else {
                    firstDirection = m_dFirstLineDirection;
                    secondDirection = m_dSecondLineDirection;
                }
                break;
        }
    } else {
        firstDirection = m_dFirstLineDirection;
        secondDirection = m_dSecondLineDirection;
    }
    
    PositionBearingDistanceMercator_Plugin( m_dCentreLat, m_dCentreLon, firstDirection, m_dFirstDistance, &l_dPrevLat, &l_dPrevLon);
    PositionBearingDistanceMercator_Plugin( m_dCentreLat, m_dCentreLon, firstDirection, m_dSecondDistance, &l_dLat, &l_dLon);
    g_pODSelect->AddSelectablePathSegment( l_dPrevLat, l_dPrevLon, l_dLat, l_dLon, m_pODPointList->GetFirst()->GetData(), NULL, this );
    l_dPrevLat = l_dLat;
    l_dPrevLon = l_dLon;
    double l_dDirection = firstDirection + l_dStepSize;
    for( int i = 0; i < l_iSteps; i++) {
        PositionBearingDistanceMercator_Plugin( m_dCentreLat, m_dCentreLon, l_dDirection, m_dSecondDistance, &l_dLat, &l_dLon);
        g_pODSelect->AddSelectablePathSegment( l_dPrevLat, l_dPrevLon, l_dLat, l_dLon, NULL, NULL, this );
        l_dPrevLat = l_dLat;
        l_dPrevLon = l_dLon;
        l_dDirection += l_dStepSize;
    }
    PositionBearingDistanceMercator_Plugin( m_dCentreLat, m_dCentreLon, secondDirection, m_dSecondDistance, &l_dLat, &l_dLon);
    g_pODSelect->AddSelectablePathSegment( l_dPrevLat, l_dPrevLon, l_dLat, l_dLon, NULL, m_pODPointList->GetLast()->GetData(), this );
    l_dPrevLat = l_dLat;
    l_dPrevLon = l_dLon;
    PositionBearingDistanceMercator_Plugin( m_dCentreLat, m_dCentreLon, secondDirection, m_dFirstDistance, &l_dLat, &l_dLon);
    g_pODSelect->AddSelectablePathSegment( l_dPrevLat, l_dPrevLon, l_dLat, l_dLon, m_pODPointList->GetLast()->GetData(), NULL, this );
    l_dPrevLat = l_dLat;
    l_dPrevLon = l_dLon;
    l_dDirection = secondDirection - l_dStepSize;
    for( int i = 0; i < l_iSteps; i++) {
        PositionBearingDistanceMercator_Plugin( m_dCentreLat, m_dCentreLon, l_dDirection, m_dFirstDistance, &l_dLat, &l_dLon);
        g_pODSelect->AddSelectablePathSegment( l_dPrevLat, l_dPrevLon, l_dLat, l_dLon, NULL, NULL, this );
        l_dPrevLat = l_dLat;
        l_dPrevLon = l_dLon;
        l_dDirection -= l_dStepSize;
    }
    PositionBearingDistanceMercator_Plugin( m_dCentreLat, m_dCentreLon, firstDirection, m_dFirstDistance, &l_dLat, &l_dLon);
    g_pODSelect->AddSelectablePathSegment( l_dPrevLat, l_dPrevLon, l_dLat, l_dLon, NULL, m_pODPointList->GetFirst()->GetData(), this );

    g_pODSelect->AddAllSelectableODPoints( this );
    
}

void GZ::MaintainWith( void )
{
    ODPoint *pStartPoint = m_pODPointList->GetFirst()->GetData();
    ODPoint *pEndPoint = m_pODPointList->GetLast()->GetData();
    m_dCentreLat = g_pfFix.Lat;
    m_dCentreLon = g_pfFix.Lon;
    
    switch(m_iMaintainWith) {
        case ID_MAINTAIN_WITH_HEADING:
            if(!isnan(g_pfFix.Hdt)) {
                PositionBearingDistanceMercator_Plugin(m_dCentreLat, m_dCentreLon, g_pfFix.Hdt + m_dFirstLineDirection, m_dFirstDistance, &pStartPoint->m_lat, &pStartPoint->m_lon);
                PositionBearingDistanceMercator_Plugin(m_dCentreLat, m_dCentreLon, g_pfFix.Hdt + m_dSecondLineDirection, m_dSecondDistance, &pEndPoint->m_lat, &pEndPoint->m_lon);
            } else {
                PositionBearingDistanceMercator_Plugin(m_dCentreLat, m_dCentreLon, m_dFirstLineDirection, m_dFirstDistance, &pStartPoint->m_lat, &pStartPoint->m_lon);
                PositionBearingDistanceMercator_Plugin(m_dCentreLat, m_dCentreLon, m_dSecondLineDirection, m_dSecondDistance, &pEndPoint->m_lat, &pEndPoint->m_lon);
            }
            break;
        case ID_MAINTAIN_WITH_COG:
            if(!isnan(g_pfFix.Cog)) {
                PositionBearingDistanceMercator_Plugin(m_dCentreLat, m_dCentreLon, g_pfFix.Cog + m_dFirstLineDirection, m_dFirstDistance, &pStartPoint->m_lat, &pStartPoint->m_lon);
                PositionBearingDistanceMercator_Plugin(m_dCentreLat, m_dCentreLon, g_pfFix.Cog + m_dSecondLineDirection, m_dSecondDistance, &pEndPoint->m_lat, &pEndPoint->m_lon);
            } else {
                PositionBearingDistanceMercator_Plugin(m_dCentreLat, m_dCentreLon, m_dFirstLineDirection, m_dFirstDistance, &pStartPoint->m_lat, &pStartPoint->m_lon);
                PositionBearingDistanceMercator_Plugin(m_dCentreLat, m_dCentreLon, m_dSecondLineDirection, m_dSecondDistance, &pEndPoint->m_lat, &pEndPoint->m_lon);
            }
            break;
    }
}

void GZ::GetLatLonPoints( PlugIn_ViewPort &piVP, wxPoint *l_pCentre, wxPoint *l_l1p1, wxPoint *l_l1p2, wxPoint *l_l2p1, wxPoint *l_l2p2 )
{
    double l_dLat;
    double l_dLon;
    double firstDirection;
    double secondDirection;
    if(m_bRotateWithBoat) {
        switch(m_iMaintainWith) {
            case ID_MAINTAIN_WITH_HEADING:
                if(!isnan(g_pfFix.Hdt)) {
                    firstDirection = g_pfFix.Hdt + m_dFirstLineDirection;
                    secondDirection = g_pfFix.Hdt + m_dSecondLineDirection;
                } else {
                    firstDirection = m_dFirstLineDirection;
                    secondDirection = m_dSecondLineDirection;
                }
                break;
            case ID_MAINTAIN_WITH_COG:
                if(!isnan(g_pfFix.Cog)) {
                    firstDirection = g_pfFix.Cog + m_dFirstLineDirection;
                    secondDirection = g_pfFix.Cog + m_dSecondLineDirection;
                } else {
                    firstDirection = m_dFirstLineDirection;
                    secondDirection = m_dSecondLineDirection;
                }
                break;
        }
    } else {
        firstDirection = m_dFirstLineDirection;
        secondDirection = m_dSecondLineDirection;
    }
    // get x, y of first point on first line
    ODPoint *l_point = m_pODPointList->GetFirst()->GetData();
    GetCanvasPixLL( &piVP, *&l_l1p1, l_point->m_lat, l_point->m_lon );
    // get x, y of second point on first line
    PositionBearingDistanceMercator_Plugin( m_dCentreLat, m_dCentreLon, firstDirection, m_dSecondDistance, &l_dLat, &l_dLon);
    GetCanvasPixLL( &piVP, *&l_l1p2, l_dLat, l_dLon );
    // get x, y of second point on second line
    l_point = m_pODPointList->GetLast()->GetData();
    GetCanvasPixLL( &piVP, *&l_l2p2, l_point->m_lat, l_point->m_lon );
    // get x, y of first point on second line
    PositionBearingDistanceMercator_Plugin( m_dCentreLat, m_dCentreLon, secondDirection, m_dFirstDistance, &l_dLat, &l_dLon);
    GetCanvasPixLL( &piVP, *&l_l2p1, l_dLat, l_dLon);
    
    GetCanvasPixLL( &piVP, *&l_pCentre, m_dCentreLat, m_dCentreLon );
}

void GZ::SetPersistence( int PersistenceType )
{
    m_iPersistenceType = PersistenceType;
    if(PersistenceType == ID_NOT_PERSISTENT || PersistenceType == ID_PERSISTENT_CRASH)
        m_bTemporary = true;
    else
        m_bTemporary = false;
}

wxBoundingBox GZ::GetBBox( void )
{
    if(!m_bNeedsUpdateBBox)
        return RBBox;
    
    double bbox_xmin = 180.;                        // set defaults
    double bbox_ymin = 90.;
    double bbox_xmax = -180;
    double bbox_ymax = -90.;
    
    RBBox.Reset();
    m_bcrosses_idl = CalculateCrossesIDL();
    
    double l_dLat;
    double l_dLon;
    double firstDirection;
    double secondDirection;
    if(m_bRotateWithBoat) {
        switch(m_iMaintainWith) {
            case ID_MAINTAIN_WITH_HEADING:
                if(!isnan(g_pfFix.Hdt)) {
                    firstDirection = g_pfFix.Hdt + m_dFirstLineDirection;
                    secondDirection = g_pfFix.Hdt + m_dSecondLineDirection;
                } else {
                    firstDirection = m_dFirstLineDirection;
                    secondDirection = m_dSecondLineDirection;
                }
                break;
            case ID_MAINTAIN_WITH_COG:
                if(!isnan(g_pfFix.Cog)) {
                    firstDirection = g_pfFix.Cog + m_dFirstLineDirection;
                    secondDirection = g_pfFix.Cog + m_dSecondLineDirection;
                } else {
                    firstDirection = m_dFirstLineDirection;
                    secondDirection = m_dSecondLineDirection;
                }
                break;
        }
    } else {
        firstDirection = m_dFirstLineDirection;
        secondDirection = m_dSecondLineDirection;
    }
    
    PositionBearingDistanceMercator_Plugin( m_dCentreLat, m_dCentreLon, firstDirection, m_dFirstDistance, &l_dLat, &l_dLon);
    PositionBearingDistanceMercator_Plugin( m_dCentreLat, m_dCentreLon, firstDirection, m_dSecondDistance, &l_dLat, &l_dLon);
    PositionBearingDistanceMercator_Plugin( m_dCentreLat, m_dCentreLon, secondDirection, m_dSecondDistance, &l_dLat, &l_dLon);
    PositionBearingDistanceMercator_Plugin( m_dCentreLat, m_dCentreLon, secondDirection, m_dFirstDistance, &l_dLat, &l_dLon);
    
    if( !m_bcrosses_idl ) {
        PositionBearingDistanceMercator_Plugin( m_dCentreLat, m_dCentreLon, firstDirection, m_dFirstDistance, &l_dLat, &l_dLon);
        if( l_dLon > bbox_xmax ) bbox_xmax = l_dLon;
        if( l_dLon < bbox_xmin ) bbox_xmin = l_dLon;
        if( l_dLat > bbox_ymax ) bbox_ymax = l_dLat;
        if( l_dLat < bbox_ymin ) bbox_ymin = l_dLat;
        PositionBearingDistanceMercator_Plugin( m_dCentreLat, m_dCentreLon, firstDirection, m_dSecondDistance, &l_dLat, &l_dLon);
        if( l_dLon > bbox_xmax ) bbox_xmax = l_dLon;
        if( l_dLon < bbox_xmin ) bbox_xmin = l_dLon;
        if( l_dLat > bbox_ymax ) bbox_ymax = l_dLat;
        if( l_dLat < bbox_ymin ) bbox_ymin = l_dLat;
        PositionBearingDistanceMercator_Plugin( m_dCentreLat, m_dCentreLon, secondDirection, m_dSecondDistance, &l_dLat, &l_dLon);
        if( l_dLon > bbox_xmax ) bbox_xmax = l_dLon;
        if( l_dLon < bbox_xmin ) bbox_xmin = l_dLon;
        if( l_dLat > bbox_ymax ) bbox_ymax = l_dLat;
        if( l_dLat < bbox_ymin ) bbox_ymin = l_dLat;
        PositionBearingDistanceMercator_Plugin( m_dCentreLat, m_dCentreLon, secondDirection, m_dFirstDistance, &l_dLat, &l_dLon);
        if( l_dLon > bbox_xmax ) bbox_xmax = l_dLon;
        if( l_dLon < bbox_xmin ) bbox_xmin = l_dLon;
        if( l_dLat > bbox_ymax ) bbox_ymax = l_dLat;
        if( l_dLat < bbox_ymin ) bbox_ymin = l_dLat;
    } else {
        //    For GZeds that cross the IDL, we compute and store
        //    the bbox as positive definite
        PositionBearingDistanceMercator_Plugin( m_dCentreLat, m_dCentreLon, firstDirection, m_dFirstDistance, &l_dLat, &l_dLon);
        if( l_dLon < 0. ) l_dLon += 360.0;
        if( l_dLon > bbox_xmax ) bbox_xmax = l_dLon;
        if( l_dLon < bbox_xmin ) bbox_xmin = l_dLon;
        if( l_dLat > bbox_ymax ) bbox_ymax = l_dLat;
        if( l_dLat < bbox_ymin ) bbox_ymin = l_dLat;
        PositionBearingDistanceMercator_Plugin( m_dCentreLat, m_dCentreLon, firstDirection, m_dSecondDistance, &l_dLat, &l_dLon);
        if( l_dLon < 0. ) l_dLon += 360.0;
        if( l_dLon > bbox_xmax ) bbox_xmax = l_dLon;
        if( l_dLon < bbox_xmin ) bbox_xmin = l_dLon;
        if( l_dLat > bbox_ymax ) bbox_ymax = l_dLat;
        if( l_dLat < bbox_ymin ) bbox_ymin = l_dLat;
        PositionBearingDistanceMercator_Plugin( m_dCentreLat, m_dCentreLon, secondDirection, m_dSecondDistance, &l_dLat, &l_dLon);
        if( l_dLon < 0. ) l_dLon += 360.0;
        if( l_dLon > bbox_xmax ) bbox_xmax = l_dLon;
        if( l_dLon < bbox_xmin ) bbox_xmin = l_dLon;
        if( l_dLat > bbox_ymax ) bbox_ymax = l_dLat;
        if( l_dLat < bbox_ymin ) bbox_ymin = l_dLat;
        PositionBearingDistanceMercator_Plugin( m_dCentreLat, m_dCentreLon, secondDirection, m_dFirstDistance, &l_dLat, &l_dLon);
        if( l_dLon < 0. ) l_dLon += 360.0;
        if( l_dLon > bbox_xmax ) bbox_xmax = l_dLon;
        if( l_dLon < bbox_xmin ) bbox_xmin = l_dLon;
        if( l_dLat > bbox_ymax ) bbox_ymax = l_dLat;
        if( l_dLat < bbox_ymin ) bbox_ymin = l_dLat;
    }
    
    RBBox.Expand( bbox_xmin, bbox_ymin );
    RBBox.Expand( bbox_xmax, bbox_ymax );
    
    m_bNeedsUpdateBBox = false;
    return RBBox;
}

