sed -i '' 's/    if (!geom::isHeightfieldExpr(fieldData, &h) || !h) return;/    \/\/ Always build regional grid for Expr leaves to accelerate large terrains.\n    if (!(fieldData.op == geom::SdfOp::Leaf \&\& fieldData.prim == geom::SdfPrim::Expr)) return;/g' src/ConstructedBeing/Singular/Object/ObjectCollision.cpp

sed -i '' 's/    _heightGrid = geom::computeHeightGrid(\*h, _fieldExtent, dimX, dimZ);/    _heightGrid = geom::computeRegionalHeightGrid(fieldData, _fieldExtent, dimX, dimZ);/g' src/ConstructedBeing/Singular/Object/ObjectCollision.cpp
