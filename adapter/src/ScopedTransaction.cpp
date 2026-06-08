#include "ScopedTransaction.h"

#include "CadDocument.h"

namespace CADNC {

ScopedTransaction::ScopedTransaction(CadDocument* doc, const char* name)
    : doc_(doc)
{
    if (doc_) doc_->openTransaction(name ? name : "Action");
}

ScopedTransaction::~ScopedTransaction()
{
    // Abort on unwind so a thrown exception between open and commit
    // never leaves a half-applied transaction in the undo history.
    if (!finished_ && doc_) {
        doc_->abortTransaction();
    }
}

void ScopedTransaction::commit()
{
    if (finished_) return;
    finished_ = true;
    if (doc_) doc_->commitTransaction();
}

void ScopedTransaction::rollback()
{
    if (finished_) return;
    finished_ = true;
    if (doc_) doc_->abortTransaction();
}

} // namespace CADNC
