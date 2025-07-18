/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2020 OpenCFD Ltd.
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

\*---------------------------------------------------------------------------*/

#include "nearWallDist.H"
#include "fvMesh.H"
#include "cellDistFuncs.H"
#include "wallFvPatch.H"
#include "surfaceFields.H"

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

void Foam::nearWallDist::calculate()
{
    cellDistFuncs wallUtils(mesh_);

    // Get patch ids of walls
    labelHashSet wallPatchIDs(wallUtils.getPatchIDs<wallPolyPatch>());

    // Size neighbours array for maximum possible

    DynamicList<label> neighbours(wallUtils.maxPatchSize(wallPatchIDs));


    // Correct all cells with face on wall

    const volVectorField& cellCentres = mesh_.C();

    forAll(mesh_.boundary(), patchi)
    {
        fvPatchScalarField& ypatch = operator[](patchi);

        const fvPatch& patch = mesh_.boundary()[patchi];

        if (isA<wallFvPatch>(patch))
        {
            const polyPatch& pPatch = patch.patch();

            const labelUList& faceCells = patch.faceCells();

            // Check cells with face on wall
            forAll(patch, patchFacei)
            {
                wallUtils.getPointNeighbours(pPatch, patchFacei, neighbours);

                label minFacei = -1;

                ypatch[patchFacei] = max // by Gill
                (
                    wallUtils.smallestDist
                    (
                        cellCentres[faceCells[patchFacei]],
                        pPatch,
                        neighbours,
                        minFacei
                    ),
                    wallDistMin_.value()
                );
            }
        }
        else
        {
            ypatch = 0.0;
        }
    }
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::nearWallDist::nearWallDist(const Foam::fvMesh& mesh)
:
    volScalarField::Boundary
    (
        mesh.boundary(),
        mesh.V(),           // Dummy internal field,
        fvPatchFieldBase::calculatedType()
    ),
    mesh_(mesh),
    wallDistMin_(dimensioned<scalar>("wallDistMin", dimLength, 0.0)) // by Gill
{
    {
        const dictionary& fvSchemes(mesh_.schemesDict());

        if (!fvSchemes.found("wallDist"))
        {
            dictionary wallDist;
            wallDist.add("default", "meshWave", true);
            const_cast<dictionary&>(fvSchemes).add("wallDist", wallDist, true);
        }

        wallDistMin_ = 
            dimensioned<scalar>::getOrAddToDict
            (
                "wallDistMin",
                const_cast<dictionary&>(fvSchemes).subDict("wallDist"),
                dimLength,
                0.0 
            );
    } // by Gill

    calculate();
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::nearWallDist::~nearWallDist()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::nearWallDist::correct()
{
    if (mesh_.topoChanging())
    {
        const DimensionedField<scalar, volMesh>& V = mesh_.V();
        const fvBoundaryMesh& bnd = mesh_.boundary();

        this->setSize(bnd.size());
        forAll(*this, patchi)
        {
            this->set
            (
                patchi,
                fvPatchField<scalar>::New
                (
                    fvPatchFieldBase::calculatedType(),
                    bnd[patchi],
                    V
                )
            );
        }
    }

    calculate();
}


// ************************************************************************* //
