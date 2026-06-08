#include "FacadeError.h"

#include <Base/Exception.h>
#include <Standard_Failure.hxx>
#include <QCoreApplication>

namespace CADNC {

FacadeError::FacadeError(Code code,
                         const QString& userMessage,
                         const QString& debugDetails)
    : std::runtime_error(userMessage.toStdString())
    , code_(code)
    , userMessage_(userMessage)
    , debugDetails_(debugDetails.isEmpty() ? userMessage : debugDetails)
{}

FacadeError FacadeError::fromFreeCADException(const Base::Exception& e)
{
    const QString detail = QString::fromUtf8(e.what());
    return FacadeError(
        Code::FreeCADException,
        QCoreApplication::translate("FacadeError", "FreeCAD error: %1").arg(detail),
        detail);
}

FacadeError FacadeError::fromOCCTException(const Standard_Failure& e)
{
    const char* msg = e.GetMessageString();
    const QString detail = QString::fromUtf8(msg ? msg : "unknown OCCT failure");
    return FacadeError(
        Code::OCCTException,
        QCoreApplication::translate("FacadeError", "Geometry kernel error: %1").arg(detail),
        detail);
}

FacadeError FacadeError::fromStdException(const std::exception& e)
{
    const QString detail = QString::fromUtf8(e.what());
    return FacadeError(
        Code::StdException,
        QCoreApplication::translate("FacadeError", "Internal error: %1").arg(detail),
        detail);
}

} // namespace CADNC
