/*---------------------------------------------------------------------------* \
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2018 OpenFOAM Foundation
    Copyright (C) 2023 OpenCFD Ltd.
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

#include "EulerDdtScheme.H"
#include "surfaceInterpolate.H"
#include "fvcDiv.H"
#include "fvMatrices.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace fv
{

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

template<class Type>
tmp<GeometricField<Type, fvPatchField, volMesh>>
EulerDdtScheme<Type>::fvcDdt
(
    const dimensioned<Type>& dt
)
{
    dimensionedScalar rDeltaT = 1.0/mesh().time().deltaT();

    IOobject ddtIOobject
    (
        "ddt("+dt.name()+')',
        mesh().time().timeName(),
        mesh().thisDb()
    );

    if (mesh().moving())
    {
        tmp<GeometricField<Type, fvPatchField, volMesh>> tdtdt
        (
            new GeometricField<Type, fvPatchField, volMesh>
            (
                ddtIOobject,
                mesh(),
                dimensioned<Type>(dt.dimensions()/dimTime, Zero)
            )
        );

        tdtdt.ref().primitiveFieldRef() =
            rDeltaT.value()*dt.value()*(1.0 - mesh().Vsc0()/mesh().Vsc());

        return tdtdt;
    }

    return tmp<GeometricField<Type, fvPatchField, volMesh>>::New
    (
        ddtIOobject,
        mesh(),
        dimensioned<Type>(dt.dimensions()/dimTime, Zero),
        fvPatchFieldBase::calculatedType()
    );
}


template<class Type>
tmp<GeometricField<Type, fvPatchField, volMesh>>
EulerDdtScheme<Type>::fvcDdt
(
    const GeometricField<Type, fvPatchField, volMesh>& vf
)
{
    dimensionedScalar rDeltaT = 1.0/mesh().time().deltaT();

    IOobject ddtIOobject
    (
        "ddt("+vf.name()+')',
        mesh().time().timeName(),
        mesh().thisDb()
    );

    if (mesh().moving())
    {
        tmp<GeometricField<Type, fvPatchField, volMesh>> tdtdt
        (
            new GeometricField<Type, fvPatchField, volMesh>
            (
                ddtIOobject,
                rDeltaT*
                (
                    vf()
                  - vf.oldTime()()*mesh().Vsc0()/mesh().Vsc()
                ),
                rDeltaT.value()*
                (
                    vf.boundaryField() - vf.oldTime().boundaryField()
                )
            )
        );

        // Different operation on boundary v.s. internal so re-evaluate
        // coupled boundaries
        tdtdt.ref().boundaryFieldRef().
            template evaluateCoupled<coupledFvPatch>();

        return tdtdt;
    }
    else
    {
        return tmp<GeometricField<Type, fvPatchField, volMesh>>
        (
            new GeometricField<Type, fvPatchField, volMesh>
            (
                ddtIOobject,
                rDeltaT*(vf - vf.oldTime())
            )
        );
    }
}


template<class Type>
tmp<GeometricField<Type, fvPatchField, volMesh>>
EulerDdtScheme<Type>::fvcDdt
(
    const dimensionedScalar& rho,
    const GeometricField<Type, fvPatchField, volMesh>& vf
)
{
    dimensionedScalar rDeltaT = 1.0/mesh().time().deltaT();

    IOobject ddtIOobject
    (
        "ddt("+rho.name()+','+vf.name()+')',
        mesh().time().timeName(),
        mesh().thisDb()
    );

    if (mesh().moving())
    {
        tmp<GeometricField<Type, fvPatchField, volMesh>> tdtdt
        (
            new GeometricField<Type, fvPatchField, volMesh>
            (
                ddtIOobject,
                rDeltaT*rho*
                (
                    vf()
                  - vf.oldTime()()*mesh().Vsc0()/mesh().Vsc()
                ),
                rDeltaT.value()*rho.value()*
                (
                    vf.boundaryField() - vf.oldTime().boundaryField()
                )
            )
        );

        // Different operation on boundary v.s. internal so re-evaluate
        // coupled boundaries
        tdtdt.ref().boundaryFieldRef().
            template evaluateCoupled<coupledFvPatch>();

        return tdtdt;
    }
    else
    {
        return tmp<GeometricField<Type, fvPatchField, volMesh>>
        (
            new GeometricField<Type, fvPatchField, volMesh>
            (
                ddtIOobject,
                rDeltaT*rho*(vf - vf.oldTime())
            )
        );
    }
}


template<class Type>
tmp<GeometricField<Type, fvPatchField, volMesh>>
EulerDdtScheme<Type>::fvcDdt
(
    const volScalarField& rho,
    const GeometricField<Type, fvPatchField, volMesh>& vf
)
{
    dimensionedScalar rDeltaT = 1.0/mesh().time().deltaT();

    IOobject ddtIOobject
    (
        "ddt("+rho.name()+','+vf.name()+')',
        mesh().time().timeName(),
        mesh().thisDb()
    );

    if (mesh().moving())
    {
        tmp<GeometricField<Type, fvPatchField, volMesh>> tdtdt
        (
            new GeometricField<Type, fvPatchField, volMesh>
            (
                ddtIOobject,
                rDeltaT*
                (
                    rho()*vf()
                  - rho.oldTime()()
                   *vf.oldTime()()*mesh().Vsc0()/mesh().Vsc()
                ),
                rDeltaT.value()*
                (
                    rho.boundaryField()*vf.boundaryField()
                  - rho.oldTime().boundaryField()
                   *vf.oldTime().boundaryField()
                )
            )
        );

        // Different operation on boundary v.s. internal so re-evaluate
        // coupled boundaries
        tdtdt.ref().boundaryFieldRef().
            template evaluateCoupled<coupledFvPatch>();

        return tdtdt;
    }
    else
    {
        return tmp<GeometricField<Type, fvPatchField, volMesh>>
        (
            new GeometricField<Type, fvPatchField, volMesh>
            (
                ddtIOobject,
                rDeltaT*(rho*vf - rho.oldTime()*vf.oldTime())
            )
        );
    }
}


template<class Type>
tmp<GeometricField<Type, fvPatchField, volMesh>>
EulerDdtScheme<Type>::fvcDdt
(
    const volScalarField& alpha,
    const volScalarField& rho,
    const GeometricField<Type, fvPatchField, volMesh>& vf
)
{
    dimensionedScalar rDeltaT = 1.0/mesh().time().deltaT();

    IOobject ddtIOobject
    (
        "ddt("+alpha.name()+','+rho.name()+','+vf.name()+')',
        mesh().time().timeName(),
        mesh().thisDb()
    );

    if (mesh().moving())
    {
        tmp<GeometricField<Type, fvPatchField, volMesh>> tdtdt
        (
            new GeometricField<Type, fvPatchField, volMesh>
            (
                ddtIOobject,
                rDeltaT*
                (
                    alpha()
                   *rho()
                   *vf()

                  - alpha.oldTime()()
                   *rho.oldTime()()
                   *vf.oldTime()()*mesh().Vsc0()/mesh().Vsc()
                ),
                rDeltaT.value()*
                (
                    alpha.boundaryField()
                   *rho.boundaryField()
                   *vf.boundaryField()

                  - alpha.oldTime().boundaryField()
                   *rho.oldTime().boundaryField()
                   *vf.oldTime().boundaryField()
                )
            )
        );

        // Different operation on boundary v.s. internal so re-evaluate
        // coupled boundaries
        tdtdt.ref().boundaryFieldRef().
            template evaluateCoupled<coupledFvPatch>();

        return tdtdt;
    }
    else
    {
        return tmp<GeometricField<Type, fvPatchField, volMesh>>
        (
            new GeometricField<Type, fvPatchField, volMesh>
            (
                ddtIOobject,
                rDeltaT
               *(
                   alpha*rho*vf
                 - alpha.oldTime()*rho.oldTime()*vf.oldTime()
                )
            )
        );
    }
}


template<class Type>
tmp<GeometricField<Type, fvsPatchField, surfaceMesh>>
EulerDdtScheme<Type>::fvcDdt
(
    const GeometricField<Type, fvsPatchField, surfaceMesh>& sf
)
{
    dimensionedScalar rDeltaT = 1.0/mesh().time().deltaT();

    IOobject ddtIOobject
    (
        "ddt("+sf.name()+')',
        mesh().time().timeName(),
        mesh().thisDb()
    );

    return tmp<GeometricField<Type, fvsPatchField, surfaceMesh>>
    (
        new GeometricField<Type, fvsPatchField, surfaceMesh>
        (
            ddtIOobject,
            rDeltaT*(sf - sf.oldTime())
        )
    );
}


template<class Type>
tmp<fvMatrix<Type>>
EulerDdtScheme<Type>::fvmDdt
(
    const GeometricField<Type, fvPatchField, volMesh>& vf
)
{
    tmp<fvMatrix<Type>> tfvm
    (
        new fvMatrix<Type>
        (
            vf,
            vf.dimensions()*dimVol/dimTime
        )
    );

    fvMatrix<Type>& fvm = tfvm.ref();

    scalar rDeltaT = 1.0/mesh().time().deltaTValue();

    fvm.diag() = rDeltaT*mesh().Vsc();

    if (mesh().moving())
    {
        fvm.source() = rDeltaT*vf.oldTime().primitiveField()*mesh().Vsc0();
    }
    else
    {
        fvm.source() = rDeltaT*vf.oldTime().primitiveField()*mesh().Vsc();
    }

    return tfvm;
}


template<class Type>
tmp<fvMatrix<Type>>
EulerDdtScheme<Type>::fvmDdt
(
    const dimensionedScalar& rho,
    const GeometricField<Type, fvPatchField, volMesh>& vf
)
{
    tmp<fvMatrix<Type>> tfvm
    (
        new fvMatrix<Type>
        (
            vf,
            rho.dimensions()*vf.dimensions()*dimVol/dimTime
        )
    );
    fvMatrix<Type>& fvm = tfvm.ref();

    scalar rDeltaT = 1.0/mesh().time().deltaTValue();

    fvm.diag() = rDeltaT*rho.value()*mesh().Vsc();

    if (mesh().moving())
    {
        fvm.source() = rDeltaT
            *rho.value()*vf.oldTime().primitiveField()*mesh().Vsc0();
    }
    else
    {
        fvm.source() = rDeltaT
            *rho.value()*vf.oldTime().primitiveField()*mesh().Vsc();
    }

    return tfvm;
}


template<class Type>
tmp<fvMatrix<Type>>
EulerDdtScheme<Type>::fvmDdt
(
    const volScalarField& rho,
    const GeometricField<Type, fvPatchField, volMesh>& vf
)
{
    tmp<fvMatrix<Type>> tfvm
    (
        new fvMatrix<Type>
        (
            vf,
            rho.dimensions()*vf.dimensions()*dimVol/dimTime
        )
    );
    fvMatrix<Type>& fvm = tfvm.ref();

    scalar rDeltaT = 1.0/mesh().time().deltaTValue();

    fvm.diag() = rDeltaT*rho.primitiveField()*mesh().Vsc();

    if (mesh().moving())
    {
        fvm.source() = rDeltaT
            *rho.oldTime().primitiveField()
            *vf.oldTime().primitiveField()*mesh().Vsc0();
    }
    else
    {
        fvm.source() = rDeltaT
            *rho.oldTime().primitiveField()
            *vf.oldTime().primitiveField()*mesh().Vsc();
    }

    return tfvm;
}


template<class Type>
tmp<fvMatrix<Type>>
EulerDdtScheme<Type>::fvmDdt
(
    const volScalarField& alpha,
    const volScalarField& rho,
    const GeometricField<Type, fvPatchField, volMesh>& vf
)
{
    tmp<fvMatrix<Type>> tfvm
    (
        new fvMatrix<Type>
        (
            vf,
            alpha.dimensions()*rho.dimensions()*vf.dimensions()*dimVol/dimTime
        )
    );
    fvMatrix<Type>& fvm = tfvm.ref();

    scalar rDeltaT = 1.0/mesh().time().deltaTValue();

    fvm.diag() =
        rDeltaT*alpha.primitiveField()*rho.primitiveField()*mesh().Vsc();

    if (mesh().moving())
    {
        fvm.source() = rDeltaT
            *alpha.oldTime().primitiveField()
            *rho.oldTime().primitiveField()
            *vf.oldTime().primitiveField()*mesh().Vsc0();
    }
    else
    {
        fvm.source() = rDeltaT
            *alpha.oldTime().primitiveField()
            *rho.oldTime().primitiveField()
            *vf.oldTime().primitiveField()*mesh().Vsc();
    }

    return tfvm;
}


template<class Type>
tmp<typename EulerDdtScheme<Type>::fluxFieldType>
EulerDdtScheme<Type>::fvcDdtUfCorr
(
    const GeometricField<Type, fvPatchField, volMesh>& U,
    const GeometricField<Type, fvsPatchField, surfaceMesh>& Uf
)
{
    dimensionedScalar rDeltaT = 1.0/mesh().time().deltaT();
/*
    if (mesh().objectRegistry::template foundObject<volScalarField>("rUAp"))
    {
        tmp<volScalarField> trAU
        (
            new volScalarField
            (
                IOobject
                (
                    "rUAp",
                    mesh().time().timeName(),
                    mesh()
                ),
                mesh().objectRegistry::template lookupObject<volScalarField>
                (
                    "rUAp"
                )
            )
        );

        const volScalarField& rAU = trAU();

        volScalarField dAU("dAU", rAU*rDeltaT);
        surfaceScalarField dAUf("dAUf", momentumInterpolate(dAU));

        tmp<fluxFieldType> toldUPhi
        (
            momentumInterpolate(dAU*U.oldTime()) & mesh().Sf()
        );

        tmp<fluxFieldType> toldPhi
        (
        	dAUf*(mesh().Sf() & Uf.oldTime())
        );

        fluxFieldType phiCorr
        (
            toldPhi() - toldUPhi()
        );

        return tmp<fluxFieldType>
        (
            new fluxFieldType
            (
                IOobject
                (
                    "ddtCorr(" + U.name() + ',' + Uf.name() + ')',
                    mesh().time().timeName(),
                    mesh().thisDb()
                ),
                this->fvcDdtPhiCoeff(U.oldTime(), toldPhi(), phiCorr)
               *phiCorr
            )
        );
    } // by Gill
    else
*/
    {
        fluxFieldType phiUf0(mesh().Sf() & Uf.oldTime());
        fluxFieldType phiCorr
        (
            phiUf0 - fvc::dotInterpolate(mesh().Sf(), U.oldTime())
        );

        return tmp<fluxFieldType>
        (
            new fluxFieldType
            (
                IOobject
                (
                    "ddtCorr(" + U.name() + ',' + Uf.name() + ')',
                    mesh().time().timeName(),
                    mesh().thisDb()
                ),
                this->fvcDdtPhiCoeff(U.oldTime(), phiUf0, phiCorr)
               *rDeltaT*phiCorr
            )
        );
    }
}


template<class Type>
tmp<typename EulerDdtScheme<Type>::fluxFieldType>
EulerDdtScheme<Type>::fvcDdtPhiCorr
(
    const GeometricField<Type, fvPatchField, volMesh>& U,
    const fluxFieldType& phi
)
{
    dimensionedScalar rDeltaT = 1.0/mesh().time().deltaT();

    if (mesh().objectRegistry::template foundObject<volScalarField>("rUAp"))
    {
        tmp<volScalarField> trAU
        (
            new volScalarField
            (
                IOobject
                (
                    "rUAp",
                    mesh().time().timeName(),
                    mesh().thisDb()
                ),
                mesh().objectRegistry::template lookupObject<volScalarField>
                (
                    "rUAp"
                )
            )
        );

        const volScalarField& rAU = trAU();

        volScalarField dAU("dAU", rAU*rDeltaT);
        surfaceScalarField dAUf("dAUf", momentumInterpolate(dAU));

        tmp<fluxFieldType> toldUPhi
        (
            momentumInterpolate(dAU*U.oldTime()) & mesh().Sf()
        );

        tmp<fluxFieldType> toldPhi
        (
            dAUf*phi.oldTime()
        );

        tmp<fluxFieldType> tPhiCorr
        (
            new fluxFieldType
            (
                IOobject
                (
                    "ddtCorr(" + U.name() + ',' + phi.name() + ')',
                    mesh().time().timeName(),
                    mesh().thisDb()
                ),
                mesh(),
                dimensioned<typename flux<Type>::type>
                (
                    "zero",
                    phi.dimensions(),
                    pTraits<typename flux<Type>::type>::zero
                )
            )
        );

        Field<typename flux<Type>::type>& phiCorr
        (
            tPhiCorr.ref().primitiveFieldRef()
        );

        phiCorr = toldPhi().primitiveField() - toldUPhi().primitiveField();

        typename GeometricField
        <
            typename flux<Type>::type, fvsPatchField, surfaceMesh
        >::Boundary& bPhiCorr = tPhiCorr.ref().boundaryFieldRef();

        const volScalarField& p
        (
            mesh().objectRegistry::template foundObject<volScalarField>
            (
                "p_rgh"
            )
          ? mesh().objectRegistry::template lookupObject<volScalarField>
            (
                "p_rgh"
            )
          : mesh().objectRegistry::template lookupObject<volScalarField>("p")
        );

        forAll(U.boundaryField(), patchi)
        {
            if
            (
                U.boundaryField()[patchi].assignable()
             || isA<fixedFluxExtrapolatedPressureFvPatchScalarField>
                (
                    p.boundaryField()[patchi]
                )
            )
            {
                bPhiCorr[patchi] =
                    toldPhi().boundaryField()[patchi]
                  - toldUPhi().boundaryField()[patchi];
            }
        }

        return tPhiCorr;
    } //by Gill
    else
    {
        fluxFieldType phiCorr
        (
            phi.oldTime() - fvc::dotInterpolate(mesh().Sf(), U.oldTime())
        );

        return tmp<fluxFieldType>
        (
            new fluxFieldType
            (
                IOobject
                (
                    "ddtCorr(" + U.name() + ',' + phi.name() + ')',
                    mesh().time().timeName(),
                    mesh().thisDb()
                ),
                this->fvcDdtPhiCoeff(U.oldTime(), phi.oldTime(), phiCorr)
               *rDeltaT*phiCorr
            )
        );
    }
}


template<class Type>
tmp<typename EulerDdtScheme<Type>::fluxFieldType>
EulerDdtScheme<Type>::fvcDdtUfCorr
(
    const volScalarField& rho,
    const GeometricField<Type, fvPatchField, volMesh>& U,
    const GeometricField<Type, fvsPatchField, surfaceMesh>& Uf
)
{
    dimensionedScalar rDeltaT = 1.0/mesh().time().deltaT();

    if
    (
        U.dimensions() == dimVelocity
     && Uf.dimensions() == rho.dimensions()*dimVelocity
    )
    {
        GeometricField<Type, fvPatchField, volMesh> rhoU0
        (
            rho.oldTime()*U.oldTime()
        );

        fluxFieldType phiUf0(mesh().Sf() & Uf.oldTime());
        fluxFieldType phiCorr(phiUf0 - fvc::dotInterpolate(mesh().Sf(), rhoU0));

        return tmp<fluxFieldType>
        (
            new fluxFieldType
            (
                IOobject
                (
                    "ddtCorr("
                  + rho.name() + ',' + U.name() + ',' + Uf.name() + ')',
                    mesh().time().timeName(),
                    mesh().thisDb()
                ),
                this->fvcDdtPhiCoeff(rhoU0, phiUf0, phiCorr, rho.oldTime())
               *rDeltaT*phiCorr
            )
        );
    }
    else if
    (
        U.dimensions() == rho.dimensions()*dimVelocity
     && Uf.dimensions() == rho.dimensions()*dimVelocity
    )
    {
        fluxFieldType phiUf0(mesh().Sf() & Uf.oldTime());
        fluxFieldType phiCorr
        (
            phiUf0 - fvc::dotInterpolate(mesh().Sf(), U.oldTime())
        );

        return tmp<fluxFieldType>
        (
            new fluxFieldType
            (
                IOobject
                (
                    "ddtCorr("
                  + rho.name() + ',' + U.name() + ',' + Uf.name() + ')',
                    mesh().time().timeName(),
                    mesh().thisDb()
                ),
                this->fvcDdtPhiCoeff
                (
                    U.oldTime(),
                    phiUf0,
                    phiCorr,
                    rho.oldTime()
                )*rDeltaT*phiCorr
            )
        );
    }
    else
    {
        FatalErrorInFunction
            << "dimensions of Uf are not correct"
            << abort(FatalError);

        return fluxFieldType::null();
    }
}


template<class Type>
tmp<typename EulerDdtScheme<Type>::fluxFieldType>
EulerDdtScheme<Type>::fvcDdtPhiCorr
(
    const volScalarField& rho,
    const GeometricField<Type, fvPatchField, volMesh>& U,
    const fluxFieldType& phi
)
{
    dimensionedScalar rDeltaT = 1.0/mesh().time().deltaT();

    if
    (
        U.dimensions() == dimVelocity
     && phi.dimensions() == rho.dimensions()*dimVelocity*dimArea
    )
    {
        if 
        (
            mesh().objectRegistry::template foundObject<volScalarField>("rUAp")
        )
        {
            tmp<volScalarField> trAU
            (
                new volScalarField
                (
                    IOobject
                    (
                        "rUAp",
                        mesh().time().timeName(),
                        mesh().thisDb()
                    ),
                    mesh().objectRegistry::template lookupObject
                    <
                        volScalarField
                    >("rUAp")
                )
            );

            const volScalarField& rAU = trAU();

            volScalarField dAU("dAU", rAU*rDeltaT);
            surfaceScalarField dAUf("dAUf", momentumInterpolate(dAU));

            const surfaceScalarField& rhof
            (
                mesh().objectRegistry::template lookupObject
                <
                    surfaceScalarField
                >("faceRho")
            );

            tmp<fluxFieldType> toldUPhi
            (
                rhof*(momentumInterpolate(dAU*U.oldTime()) & mesh().Sf())
            );

            tmp<fluxFieldType> toldPhi(dAUf*phi.oldTime());

            tmp<fluxFieldType> tPhiCorr
            (
                new fluxFieldType
                (
                    IOobject
                    (    
                        "ddtCorr(" 
                      + rho.name() + ',' + U.name() + ',' + phi.name() + ')',
                        mesh().time().timeName(),
                        mesh().thisDb()
                    ),
                    mesh(),
                    dimensioned<typename flux<Type>::type>
                    (
                        "zero",
                        phi.dimensions(),
                        pTraits<typename flux<Type>::type>::zero
                    )
                )
            );

            Field<typename flux<Type>::type>& phiCorr
            (
                tPhiCorr.ref().primitiveFieldRef()
            );

            phiCorr = toldPhi().primitiveField() - toldUPhi().primitiveField();

            typename GeometricField
            <
                typename flux<Type>::type, fvsPatchField, surfaceMesh
            >::Boundary& bPhiCorr = tPhiCorr.ref().boundaryFieldRef();

            const volScalarField& p
            (
                mesh().objectRegistry::template foundObject<volScalarField>
                (
                    "p_rgh"
                )
             ?  mesh().objectRegistry::template lookupObject<volScalarField>
                (
                    "p_rgh"
                )
             :  mesh().objectRegistry::template lookupObject<volScalarField>
                (
                    "p"
                )
            );

            forAll(U.boundaryField(), patchi)
            {
                if
                (
                    U.boundaryField()[patchi].assignable()
                 || isA<fixedFluxExtrapolatedPressureFvPatchScalarField>
                    (
                        p.boundaryField()[patchi]
                    )
                )
                {
                    bPhiCorr[patchi] =
                    (
                        toldPhi().boundaryField()[patchi]
                      - toldUPhi().boundaryField()[patchi]
                    );
                }
            }

            return tPhiCorr;
        } // by Gill
        else
        {
            GeometricField<Type, fvPatchField, volMesh> rhoU0
            (
                rho.oldTime()*U.oldTime()
            );

            fluxFieldType phiCorr
            (
                phi.oldTime() - fvc::dotInterpolate(mesh().Sf(), rhoU0)
            );

            return tmp<fluxFieldType>
            (
                new fluxFieldType
                (
                    IOobject
                    (
                        "ddtCorr("
                      + rho.name() + ',' + U.name() + ',' + phi.name() + ')',
                        mesh().time().timeName(),
                        mesh().thisDb()
                    ),
                    this->fvcDdtPhiCoeff
                    (
                        rhoU0,
                        phi.oldTime(),
                        phiCorr,
                        rho.oldTime()
                    )*rDeltaT*phiCorr
                )
            );
        }
    }
    else if
    (
        U.dimensions() == rho.dimensions()*dimVelocity
     && phi.dimensions() == rho.dimensions()*dimVelocity*dimArea
    )
    {
        fluxFieldType phiCorr
        (
            phi.oldTime() - fvc::dotInterpolate(mesh().Sf(), U.oldTime())
        );

        return tmp<fluxFieldType>
        (
            new fluxFieldType
            (
                IOobject
                (
                    "ddtCorr("
                  + rho.name() + ',' + U.name() + ',' + phi.name() + ')',
                    mesh().time().timeName(),
                    mesh().thisDb()
                ),
                this->fvcDdtPhiCoeff
                (
                    U.oldTime(),
                    phi.oldTime(),
                    phiCorr,
                    rho.oldTime()
                )*rDeltaT*phiCorr
            )
        );
    }
    else
    {
        FatalErrorInFunction
            << "dimensions of phi are not correct"
            << abort(FatalError);

        return fluxFieldType::null();
    }
}


template<class Type>
tmp<surfaceScalarField> EulerDdtScheme<Type>::meshPhi
(
    const GeometricField<Type, fvPatchField, volMesh>&
)
{
    return mesh().phi();
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace fv

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //
