#pragma once

/**
 * @file ScopedTransaction.h
 * @brief RAII wrapper around FreeCAD undo transactions.
 *
 * Per WRAPPER_CONTRACT § 2.5 — every mutating engine method opens one.
 *
 * Usage:
 *     ScopedTransaction tx(document_.get(), "Add Line");
 *     int id = facade_->addLine(...);      // may throw FacadeError
 *     tx.commit();                          // finalize on success
 *     // If an exception unwinds before commit(), the destructor aborts.
 */

namespace CADNC {

class CadDocument;

class ScopedTransaction {
public:
    ScopedTransaction(CadDocument* doc, const char* name);
    ~ScopedTransaction();

    ScopedTransaction(const ScopedTransaction&) = delete;
    ScopedTransaction& operator=(const ScopedTransaction&) = delete;

    /// Finalize: transaction joins the undo history. Safe to call twice.
    void commit();

    /// Discard: transaction removed from undo history. Safe to call twice.
    void rollback();

    /// True after commit() or rollback().
    bool isFinished() const noexcept { return finished_; }

private:
    CadDocument* doc_;   // non-owning; may be null if document destroyed
    bool finished_ = false;
};

} // namespace CADNC
