#pragma once

/**
 * @file FacadeError.h
 * @brief Unified error type for CADNC facade methods.
 *
 * Per WRAPPER_CONTRACT § 2.3 — facades throw FacadeError for every failure
 * mode. CadEngine catches at Q_INVOKABLE boundary, translates to
 * errorOccurred(QString) signal, returns a failure value to QML.
 */

#include <QString>
#include <exception>
#include <stdexcept>

namespace Base { class Exception; }
class Standard_Failure;

namespace CADNC {

class FacadeError : public std::runtime_error {
public:
    enum class Code {
        NoActiveDocument,
        ObjectNotFound,
        InvalidArgument,
        ConstraintConflict,
        GeometryInvalid,
        FreeCADException,
        OCCTException,
        StdException,
        PythonError,
        Unknown,
    };

    FacadeError(Code code,
                const QString& userMessage,
                const QString& debugDetails = QString());

    Code code() const noexcept { return code_; }
    QString userMessage() const noexcept { return userMessage_; }
    QString debugDetails() const noexcept { return debugDetails_; }

    static FacadeError fromFreeCADException(const Base::Exception& e);
    static FacadeError fromOCCTException(const Standard_Failure& e);
    static FacadeError fromStdException(const std::exception& e);

private:
    Code code_;
    QString userMessage_;
    QString debugDetails_;
};

} // namespace CADNC
