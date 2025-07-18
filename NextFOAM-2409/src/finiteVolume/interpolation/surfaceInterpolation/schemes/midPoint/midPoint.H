/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Class
    Foam::midPoint

Group
    grpFvSurfaceInterpolationSchemes

Description
    Mid-point interpolation (weighting factors = 0.5) scheme class.

SourceFiles
    midPoint.C

\*---------------------------------------------------------------------------*/

#ifndef midPoint_H
#define midPoint_H

#include "surfaceInterpolationScheme.H"
#include "volFields.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                           Class midPoint Declaration
\*---------------------------------------------------------------------------*/

template<class Type>
class midPoint
:
    public surfaceInterpolationScheme<Type>
{
    // Private Member Functions

        //- No copy assignment
        void operator=(const midPoint&) = delete;


public:

    //- Runtime type information
    TypeName("midPoint");


    // Constructors

        //- Construct from mesh
        midPoint(const fvMesh& mesh)
        :
            surfaceInterpolationScheme<Type>(mesh)
        {}

        //- Construct from Istream
        midPoint(const fvMesh& mesh, Istream&)
        :
            surfaceInterpolationScheme<Type>(mesh)
        {}

        //- Construct from faceFlux and Istream
        midPoint
        (
            const fvMesh& mesh,
            const surfaceScalarField&,
            Istream&
        )
        :
            surfaceInterpolationScheme<Type>(mesh)
        {}


    // Member Functions

        //- Return the interpolation weighting factors
        tmp<surfaceScalarField> weights
        (
            const GeometricField<Type, fvPatchField, volMesh>&
        ) const
        {
            tmp<surfaceScalarField> taw
            (
                new surfaceScalarField
                (
                    IOobject
                    (
                        "midPointWeights",
                        this->mesh().time().timeName(),
                        this->mesh().thisDb(),
                        IOobject::NO_READ,
                        IOobject::NO_WRITE,
                        IOobject::NO_REGISTER
                    ),
                    this->mesh(),
                    dimensionedScalar("0.5", dimless, 0.5)
                )
            );

            surfaceScalarField::Boundary& awBf =
                taw.ref().boundaryFieldRef();

            forAll(awBf, patchi)
            {
                if (!awBf[patchi].coupled())
                {
                    awBf[patchi] = 1.0;
                }
            }

            return taw;
        }
};


template<class Type>
tmp<GeometricField<Type, fvsPatchField, surfaceMesh>>
midPointInterpolate(const GeometricField<Type, fvPatchField, volMesh>& vf)
{
    return surfaceInterpolationScheme<Type>::interpolate
           (
               vf, 
               tmp<surfaceScalarField>
               (
                   new surfaceScalarField
                   (
                       IOobject
                       (
                           "midPointWeights",
                           vf.mesh().time().timeName(),
                           vf.mesh(),
                           IOobject::NO_READ,
                           IOobject::NO_WRITE,
                           false
                       ),
                       vf.mesh(),
                       dimensionedScalar("0.5", dimless, 0.5)
                   )
               )
           );
}


template<class Type>
tmp<GeometricField<Type, fvsPatchField, surfaceMesh>>
midPointInterpolate
(
    const tmp<GeometricField<Type, fvPatchField, volMesh>>& tvf
)
{
    tmp<GeometricField<Type, fvsPatchField, surfaceMesh>> tinterp =
        midPointInterpolate(tvf());
    tvf.clear();
    return tinterp;
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
