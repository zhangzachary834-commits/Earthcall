#include "Singularity/OntoMath/LinearAlgebra.hpp"

#include <algorithm>
#include <cmath>
#include <glm/gtc/quaternion.hpp>

namespace OntoMath {
namespace {
bool finiteVec3(const glm::vec3& v) {
    return std::isfinite(static_cast<double>(v.x)) &&
           std::isfinite(static_cast<double>(v.y)) &&
           std::isfinite(static_cast<double>(v.z));
}
std::optional<glm::vec3> applyHomogeneous(const MatrixValue& m,
                                           const glm::vec3& v,
                                           double w) {
    if (!m.valid() || m.rows()!=4 || m.cols()!=4 || !finiteVec3(v)) return std::nullopt;
    const double in[4]={v.x,v.y,v.z,w};
    double out[4]={0,0,0,0};
    for (std::size_t r=0;r<4;++r) {
        for (std::size_t c=0;c<4;++c) out[r]+=m.at(r,c)*in[c];
        if (!std::isfinite(out[r])) return std::nullopt;
    }
    const double tol=1e-10*std::max(1.0,std::max(std::abs(w),std::abs(out[3])));
    if (std::abs(out[3]-w)>tol) return std::nullopt;
    return glm::vec3(static_cast<float>(out[0]),static_cast<float>(out[1]),static_cast<float>(out[2]));
}
} // namespace

std::optional<MatrixValue> affineIdentity() { return matrixIdentity(4); }

std::optional<MatrixValue> affineTranslation(const glm::vec3& t) {
    if (!finiteVec3(t)) return std::nullopt;
    auto m=matrixIdentity(4); if (!m) return std::nullopt;
    m->at(0,3)=t.x; m->at(1,3)=t.y; m->at(2,3)=t.z; return m;
}
std::optional<MatrixValue> affineScale(const glm::vec3& s) {
    if (!finiteVec3(s)) return std::nullopt;
    auto m=matrixIdentity(4); if (!m) return std::nullopt;
    m->at(0,0)=s.x; m->at(1,1)=s.y; m->at(2,2)=s.z; return m;
}
std::optional<MatrixValue> affineAxisAngle(const glm::vec3& axis,double radians) {
    if (!finiteVec3(axis)||!std::isfinite(radians)) return std::nullopt;
    double x=axis.x,y=axis.y,z=axis.z;
    const double len=std::sqrt(x*x+y*y+z*z);
    if (!std::isfinite(len)||len<=kMatrixRelativePivotEpsilon) return std::nullopt;
    x/=len; y/=len; z/=len;
    const double c=std::cos(radians), s=std::sin(radians), t=1.0-c;
    return MatrixValue::create(4,4,{
        t*x*x+c,t*x*y-s*z,t*x*z+s*y,0,
        t*x*y+s*z,t*y*y+c,t*y*z-s*x,0,
        t*x*z-s*y,t*y*z+s*x,t*z*z+c,0,
        0,0,0,1});
}
std::optional<MatrixValue> affineEulerXYZDegrees(const glm::vec3& d) {
    if (!finiteVec3(d)) return std::nullopt;
    constexpr double q=3.14159265358979323846/180.0;
    auto x=affineAxisAngle({1,0,0},d.x*q), y=affineAxisAngle({0,1,0},d.y*q),
         z=affineAxisAngle({0,0,1},d.z*q);
    if (!x||!y||!z) return std::nullopt;
    auto xy=matrixMultiply(*x,*y); return xy?matrixMultiply(*xy,*z):std::nullopt;
}
std::optional<MatrixValue> affineCompose(const MatrixValue& a,const MatrixValue& b) {
    if (!a.valid()||!b.valid()||a.rows()!=4||a.cols()!=4||b.rows()!=4||b.cols()!=4)
        return std::nullopt;
    return matrixMultiply(a,b);
}
std::optional<MatrixValue> affineTRS(const glm::vec3& t,const glm::vec3& r,const glm::vec3& s) {
    auto tm=affineTranslation(t), rm=affineEulerXYZDegrees(r), sm=affineScale(s);
    if (!tm||!rm||!sm) return std::nullopt;
    auto tr=affineCompose(*tm,*rm); return tr?affineCompose(*tr,*sm):std::nullopt;
}
std::optional<glm::vec3> transformPoint(const MatrixValue& m,const glm::vec3& p) {
    return applyHomogeneous(m,p,1.0);
}
std::optional<glm::vec3> transformDirection(const MatrixValue& m,const glm::vec3& d) {
    return applyHomogeneous(m,d,0.0);
}
std::optional<glm::vec3> pullbackCovector(const MatrixValue& m,const glm::vec3& d) {
    if (!m.valid()||m.rows()!=4||m.cols()!=4||!finiteVec3(d)) return std::nullopt;
    std::vector<double> e(9);
    for(std::size_t r=0;r<3;++r)
        for(std::size_t c=0;c<3;++c)
            e[r*3+c]=m.at(r,c);
    auto linear=MatrixValue::create(3,3,std::move(e));
    if(!linear) return std::nullopt;
    auto transposed=matrixTranspose(*linear);
    return transposed?matrixMultiplyVec3(*transposed,d):std::nullopt;
}
std::optional<glm::vec3> affineExtractTranslation(const MatrixValue& m) {
    if (!m.valid() || m.rows() != 4 || m.cols() != 4) return std::nullopt;
    glm::vec3 t(static_cast<float>(m.at(0, 3)),
                static_cast<float>(m.at(1, 3)),
                static_cast<float>(m.at(2, 3)));
    if (!finiteVec3(t)) return std::nullopt;
    return t;
}

std::optional<glm::vec3> affineExtractScale(const MatrixValue& m) {
    if (!m.valid() || m.rows() != 4 || m.cols() != 4) return std::nullopt;
    glm::vec3 scale(0.0f);
    for (std::size_t col = 0; col < 3; ++col) {
        double squaredLength = 0.0;
        for (std::size_t row = 0; row < 3; ++row) {
            const double component = m.at(row, col);
            squaredLength += component * component;
        }
        const double length = std::sqrt(squaredLength);
        if (!std::isfinite(length)) return std::nullopt;
        scale[static_cast<int>(col)] = static_cast<float>(length);
    }
    return finiteVec3(scale) ? std::optional<glm::vec3>(scale) : std::nullopt;
}

std::optional<glm::vec3> affineExtractEulerXYZDegrees(const MatrixValue& m) {
    const auto extracted = affineExtractScale(m);
    if (!extracted) return std::nullopt;
    glm::vec3 scale = *extracted;
    if (scale.x <= 1e-6f) scale.x = 1.0f;
    if (scale.y <= 1e-6f) scale.y = 1.0f;
    if (scale.z <= 1e-6f) scale.z = 1.0f;
    glm::mat3 basis;
    for (int col = 0; col < 3; ++col)
        for (int row = 0; row < 3; ++row)
            basis[col][row] = static_cast<float>(m.at(row, col)) / scale[col];
    if (glm::determinant(basis) < 0.0f) basis[0] = -basis[0];
    const glm::vec3 degrees =
        glm::degrees(glm::eulerAngles(glm::normalize(glm::quat_cast(basis))));
    return finiteVec3(degrees) ? std::optional<glm::vec3>(degrees) : std::nullopt;
}

std::optional<MatrixValue> affineExtractRotationBasis(const MatrixValue& m) {
    const auto scale = affineExtractScale(m);
    if (!scale ||
        scale->x <= kMatrixRelativePivotEpsilon ||
        scale->y <= kMatrixRelativePivotEpsilon ||
        scale->z <= kMatrixRelativePivotEpsilon) {
        return std::nullopt;
    }
    auto rotation = matrixIdentity(4);
    if (!rotation) return std::nullopt;
    for (std::size_t col = 0; col < 3; ++col) {
        const double divisor = static_cast<double>((*scale)[static_cast<int>(col)]);
        for (std::size_t row = 0; row < 3; ++row) {
            rotation->at(row, col) = m.at(row, col) / divisor;
        }
    }
    return rotation;
}

std::optional<MatrixValue> affineSelectTRS(const MatrixValue& parent,
                                           const MatrixValue& child,
                                           const MatrixValue& localOffset,
                                           bool inheritTranslation,
                                           bool inheritRotation,
                                           bool inheritScale) {
    if (!parent.valid() || !child.valid() || !localOffset.valid() ||
        parent.rows()!=4 || parent.cols()!=4 || child.rows()!=4 || child.cols()!=4 ||
        localOffset.rows()!=4 || localOffset.cols()!=4) return std::nullopt;

    const auto parentLocal=affineCompose(parent, localOffset);
    if (!parentLocal) return std::nullopt;

    const auto translation=inheritTranslation
        ? transformPoint(parent, *affineExtractTranslation(localOffset))
        : affineExtractTranslation(child);
    const auto rotation=affineExtractRotationBasis(inheritRotation ? parent : child);
    const auto scale=affineExtractScale(inheritScale ? *parentLocal : child);
    if (!translation || !rotation || !scale) return std::nullopt;

    const auto tm=affineTranslation(*translation);
    const auto sm=affineScale(*scale);
    if (!tm || !sm) return std::nullopt;
    const auto tr=affineCompose(*tm, *rotation);
    return tr ? affineCompose(*tr, *sm) : std::nullopt;
}

std::optional<MatrixValue> inverseAffine(const MatrixValue& m) {
    if (!m.valid()||m.rows()!=4||m.cols()!=4) return std::nullopt;
    constexpr double e=1e-10;
    if (std::abs(m.at(3,0))>e||std::abs(m.at(3,1))>e||std::abs(m.at(3,2))>e||
        std::abs(m.at(3,3)-1.0)>e) return std::nullopt;
    return matrixInverse(m);
}
std::optional<glm::vec3> transformNormal(const MatrixValue& m,const glm::vec3& n) {
    if (!m.valid()||m.rows()!=4||m.cols()!=4||!finiteVec3(n)) return std::nullopt;
    std::vector<double> e(9);
    for(std::size_t r=0;r<3;++r) for(std::size_t c=0;c<3;++c) e[r*3+c]=m.at(r,c);
    auto l=MatrixValue::create(3,3,std::move(e)); if(!l) return std::nullopt;
    auto inv=matrixInverse(*l); if(!inv) return std::nullopt;
    auto tr=matrixTranspose(*inv); return tr?matrixMultiplyVec3(*tr,n):std::nullopt;
}
} // namespace OntoMath
