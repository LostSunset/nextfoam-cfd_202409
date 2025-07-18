    label i(pvCoupling.solidIndex());

    fvMesh& mesh = solidRegions[i];

    solidThermo& thermo = thermoSolid[i];

    tmp<volScalarField> trho = thermo.rho();
    const volScalarField& rho = trho();

    tmp<volScalarField> tcp = thermo.Cp();
    const volScalarField& cp = tcp();

    tmp<volSymmTensorField> taniAlpha;
    if (!thermo.isotropic())
    {
        volSymmTensorField& aniAlpha = aniAlphas[i];
        tmp<volVectorField> tkappaByCp = thermo.Kappa()/cp;
        const coordinateSystem& coodSys = coordinates[i];

        aniAlpha.primitiveFieldRef() =
            coodSys.transformPrincipal
            (
                mesh.cellCentres(),
                tkappaByCp()
            );

        aniAlpha.correctBoundaryConditions();

        taniAlpha = tmp<volSymmTensorField>
        (
            new volSymmTensorField(aniAlpha)
        );
    }


    volScalarField& h = thermo.he();

    const volScalarField& betav = betavSolid[i];

    fv::options& fvOptions = solidFvOptions[i];

    autoPtr<radiation::radiationModel>& radiation = radiationSolid[i];
    bool& radiationEnabled = radiationEnabledSolid[i];
