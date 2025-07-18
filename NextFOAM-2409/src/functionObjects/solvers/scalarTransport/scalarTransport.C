/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2012-2017 OpenFOAM Foundation
    Copyright (C) 2015-2023 OpenCFD Ltd.
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

#include "scalarTransport.H"
#include "surfaceFields.H"
#include "fvmDdt.H"
#include "fvmDiv.H"
#include "fvmLaplacian.H"
#include "fvmSup.H"
#include "CMULES.H"
#include "turbulentTransportModel.H"
#include "turbulentFluidThermoModel.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace functionObjects
{
    defineTypeNameAndDebug(scalarTransport, 0);

    addToRunTimeSelectionTable
    (
        functionObject,
        scalarTransport,
        dictionary
    );
}
}


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

Foam::volScalarField& Foam::functionObjects::scalarTransport::transportedField()
{
    if (!foundObject<volScalarField>(fieldName_))
    {
        auto tfldPtr = tmp<volScalarField>::New
        (
            IOobject
            (
                fieldName_,
                mesh_.time().timeName(),
                mesh_,
                IOobject::MUST_READ,
                IOobject::AUTO_WRITE,
                IOobject::REGISTER
            ),
            mesh_
        );
        store(fieldName_, tfldPtr);

        if (phaseName_ != "none")
        {
            mesh_.setFluxRequired(fieldName_);
        }
    }

    return lookupObjectRef<volScalarField>(fieldName_);
}


Foam::tmp<Foam::volScalarField> Foam::functionObjects::scalarTransport::D
(
    const volScalarField& s,
    const surfaceScalarField& phi
) const
{
    const word Dname("D" + s.name());

    if (constantD_)
    {
        return tmp<volScalarField>::New
        (
            IOobject
            (
                Dname,
                mesh_.time().timeName(),
                mesh_.time(),
                IOobject::NO_READ,
                IOobject::NO_WRITE
            ),
            mesh_,
            dimensionedScalar(Dname, phi.dimensions()/dimLength, D_)
        );
    }

    if (nutName_ != "none")
    {
        const volScalarField& nutMean =
            mesh_.lookupObject<volScalarField>(nutName_);

        return tmp<volScalarField>::New(Dname, nutMean);
    }

    // Incompressible
    {
        const auto* turb =
            findObject<incompressible::turbulenceModel>
            (
                turbulenceModel::propertiesName
            );

        if (turb)
        {
            return tmp<volScalarField>::New
            (
                Dname,
                alphaD_ * turb->nu() + alphaDt_ * turb->nut()
            );
        }
    }

    // Compressible
    {
        const auto* turb =
            findObject<compressible::turbulenceModel>
            (
                turbulenceModel::propertiesName
            );

        if (turb)
        {
            return tmp<volScalarField>::New
            (
                Dname,
                alphaD_ * turb->mu() + alphaDt_ * turb->mut()
            );
        }
    }


    return tmp<volScalarField>::New
    (
        IOobject
        (
            Dname,
            mesh_.time().timeName(),
            mesh_.time(),
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh_,
        dimensionedScalar(phi.dimensions()/dimLength, Zero)
    );
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::functionObjects::scalarTransport::scalarTransport
(
    const word& name,
    const Time& runTime,
    const dictionary& dict
)
:
    fvMeshFunctionObject(name, runTime, dict),
    fieldName_(dict.getOrDefault<word>("field", "s")),
    phiName_(dict.getOrDefault<word>("phi", "phi")),
    rhoName_(dict.getOrDefault<word>("rho", "rho")),
    nutName_(dict.getOrDefault<word>("nut", "none")),
    phaseName_(dict.getOrDefault<word>("phase", "none")),
    phasePhiCompressedName_
    (
        dict.getOrDefault<word>("phasePhiCompressed", "alphaPhiUn")
    ),
    D_(0),
    constantD_(false),
    nCorr_(0),
    resetOnStartUp_(false),
    schemesField_("unknown-schemesField"),
    fvOptions_(mesh_),
    bounded01_(dict.getOrDefault("bounded01", true))
{
    read(dict);

    // Force creation of transported field so any BCs using it can
    // look it up
    volScalarField& s = transportedField();

    if (resetOnStartUp_)
    {
        s == dimensionedScalar(dimless, Zero);
    }
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::functionObjects::scalarTransport::~scalarTransport()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::functionObjects::scalarTransport::read(const dictionary& dict)
{
    fvMeshFunctionObject::read(dict);

    dict.readIfPresent("phi", phiName_);
    dict.readIfPresent("rho", rhoName_);
    dict.readIfPresent("nut", nutName_);
    dict.readIfPresent("phase", phaseName_);
    dict.readIfPresent("bounded01", bounded01_);

    schemesField_ = dict.getOrDefault("schemesField", fieldName_);
    constantD_ = dict.readIfPresent("D", D_);
    alphaD_ = dict.getOrDefault<scalar>("alphaD", 1);
    alphaDt_ = dict.getOrDefault<scalar>("alphaDt", 1);

    dict.readIfPresent("nCorr", nCorr_);
    dict.readIfPresent("resetOnStartUp", resetOnStartUp_);

    if (dict.found("fvOptions"))
    {
        fvOptions_.reset(dict.subDict("fvOptions"));
    }

    return true;
}


bool Foam::functionObjects::scalarTransport::execute()
{
    volScalarField& s = transportedField();

    Log << type() << " execute: " << s.name() << endl;

    const surfaceScalarField& phi =
        mesh_.lookupObject<surfaceScalarField>(phiName_);

    // Calculate the diffusivity
    volScalarField D(this->D(s, phi));

    word divScheme("div(phi," + schemesField_ + ")");
    word laplacianScheme("laplacian(" + D.name() + "," + schemesField_ + ")");

    // Set under-relaxation coeff
    scalar relaxCoeff = 0;
    mesh_.relaxEquation(schemesField_, relaxCoeff);

    // Two phase scalar transport
    if (phaseName_ != "none")
    {
        const volScalarField& alpha =
            mesh_.lookupObject<volScalarField>(phaseName_);

        const surfaceScalarField& limitedPhiAlpha =
            mesh_.lookupObject<surfaceScalarField>(phasePhiCompressedName_);

        D *= pos(alpha - 0.99);

        // Reset D dimensions consistent with limitedPhiAlpha
        D.dimensions().reset(limitedPhiAlpha.dimensions()/dimLength);

        // Solve
        tmp<surfaceScalarField> tTPhiUD;
        for (label i = 0; i <= nCorr_; i++)
        {
            fvScalarMatrix sEqn
            (
                fvm::ddt(s)
              + fvm::div(limitedPhiAlpha, s, divScheme)
              - fvm::laplacian(D, s, laplacianScheme)
              ==
                alpha*fvOptions_(s)
            );

            sEqn.relax(relaxCoeff);
            fvOptions_.constrain(sEqn);
            sEqn.solve(schemesField_);

            tTPhiUD = sEqn.flux();
        }

        if (bounded01_)
        {
            MULES::explicitSolve
            (
                geometricOneField(),
                s,
                phi,
                tTPhiUD.ref(),
                oneField(),
                zeroField()
            );
        }
    }
    else if (phi.dimensions() == dimMass/dimTime)
    {
        const volScalarField& rho = lookupObject<volScalarField>(rhoName_);

        for (label i = 0; i <= nCorr_; i++)
        {

            fvScalarMatrix sEqn
            (
                fvm::ddt(rho, s)
              + fvm::div(phi, s, divScheme)
              - fvm::laplacian(D, s, laplacianScheme)
             ==
                fvOptions_(rho, s)
            );

            sEqn.relax(relaxCoeff);

            fvOptions_.constrain(sEqn);

            sEqn.solve(schemesField_);
        }
    }
    else if (phi.dimensions() == dimVolume/dimTime)
    {
        for (label i = 0; i <= nCorr_; i++)
        {
            fvScalarMatrix sEqn
            (
                fvm::ddt(s)
              + fvm::div(phi, s, divScheme)
              - fvm::laplacian(D, s, laplacianScheme)
             ==
                fvOptions_(s)
            );

            sEqn.relax(relaxCoeff);

            fvOptions_.constrain(sEqn);

            sEqn.solve(schemesField_);
        }
    }
    else
    {
        FatalErrorInFunction
            << "Incompatible dimensions for phi: " << phi.dimensions() << nl
            << "Dimensions should be " << dimMass/dimTime << " or "
            << dimVolume/dimTime << exit(FatalError);
    }

    Log << endl;

    return true;
}


bool Foam::functionObjects::scalarTransport::write()
{
    Log << type() << " write: " << name() << nl
        << tab << fieldName_ << nl
        << endl;

    volScalarField& s = transportedField();

    s.write();

    return true;
}


// ************************************************************************* //
