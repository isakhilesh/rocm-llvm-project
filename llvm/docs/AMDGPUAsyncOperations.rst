===============================
 AMDGPU Asynchronous Operations
===============================

.. contents::
   :local:

Introduction
============

Asynchronous operations are memory transfers (usually between the global memory
and LDS) that are completed independently at an unspecified scope. A thread that
requests one or more asynchronous transfers can use *async markers* to track
their completion. The thread waits for each marker to be *completed*, which
indicates that requests initiated in program order before this marker have also
completed.

Operations
==========

``async_load_to_lds``
---------------------

.. code-block:: llvm

  ; Legacy "LDS DMA" operations
  void @llvm.amdgcn.load.to.lds(ptr %src, ptr %dst, ASYNC)
  void @llvm.amdgcn.global.load.lds(ptr %src, ptr %dst, ASYNC)
  void @llvm.amdgcn.raw.buffer.load.lds(ptr %src, ptr %dst, ASYNC)
  void @llvm.amdgcn.raw.ptr.buffer.load.lds(ptr %src, ptr %dst, ASYNC)
  void @llvm.amdgcn.struct.buffer.load.lds(ptr %src, ptr %dst, ASYNC)
  void @llvm.amdgcn.struct.ptr.buffer.load.lds(ptr %src, ptr %dst, ASYNC)

Requests an async operation that copies the specified number of bytes from the
global/buffer pointer ``%src`` to the LDS pointer ``%dst``.

The optional parameter `ASYNC` is a bit in the auxiliary argument to those
intrinsics, as documented in :ref:`LDS DMA operations<amdgpu-lds-dma-bits>`.
When set, it indicates that the compiler should not automatically track the
completion of this operation.

``@llvm.amdgcn.asyncmark()``
----------------------------

Creates an *async marker* to track all the async operations that are program
ordered before this call. A marker M is said to be *completed* only when all
async operations program ordered before M are reported by the implementation as
having finished, and it is said to be *outstanding* otherwise.

Thus we have the following sufficient condition:

  An async operation X is *completed* at a program point P if there exists a
  marker M such that X is program ordered before M, M is program ordered before
  P, and M is completed. X is said to be *outstanding* at P otherwise.

``@llvm.amdgcn.wait.asyncmark(i32 %N)``
---------------------------------------

Waits until the ``N+1`` th predecessor marker M in program order before this
call is completed, if M exists.

N is an unsigned integer; the ``N+1`` th predecessor marker of point X is a
marker M such that there are `N` markers in program order from M to X, not
including M.

Memory Consistency Model
========================

Each asynchronous operation consists of a non-atomic read on the source and a
non-atomic write on the destination. Legacy "LDS DMA" intrinsics result in async
accesses that guarantee visibility relative to other memory operations as
follows:

  An asynchronous operation `A` program ordered before an overlapping memory
  operation `X` happens-before `X` if `A` is completed before `X`.

  A memory operation `X` program ordered before an overlapping asynchronous
  operation `A` happens-before `A`.

Function calls in LLVM
======================

The underlying abstract machine does not implicitly track the completion of
async operations while entering or returning from a function call.

.. note::

   As long as the caller uses sufficient waitcnts to track its own async
   operations, the actions performed by the callee cannot affect correctness,
   but the resulting implementation may contain redundant waits.

Examples
========

Uneven blocks of async transfers
--------------------------------

.. code-block:: c++

   void foo(global int *g, local int *l) {
     // first block
     async_load_to_lds(l, g);
     async_load_to_lds(l, g);
     async_load_to_lds(l, g);
     asyncmark();

     // second block; longer
     async_load_to_lds(l, g);
     async_load_to_lds(l, g);
     async_load_to_lds(l, g);
     async_load_to_lds(l, g);
     async_load_to_lds(l, g);
     asyncmark();

     // third block; shorter
     async_load_to_lds(l, g);
     async_load_to_lds(l, g);
     asyncmark();

     // Wait for first block
     wait.asyncmark(2);
   }

Software pipeline
-----------------

.. code-block:: c++

   void foo(global int *g, local int *l) {
     // first block
     asyncmark();

     // second block
     asyncmark();

     // third block
     asyncmark();

     for (;;) {
       wait.asyncmark(2);
       // use data

       // next block
       asyncmark();
     }

     // flush one block
     wait.asyncmark(2);

     // flush one more block
     wait.asyncmark(1);

     // flush last block
     wait.asyncmark(0);
   }

Ordinary function call
----------------------

.. code-block:: c++

   extern void bar(); // may or may not make async calls

   void foo(global int *g, local int *l) {
       // first block
       asyncmark();

       // second block
       asyncmark();

       // function call
       bar();

       // third block
       asyncmark();

       wait.asyncmark(1); // will wait for at least the second block, possibly including bar()
       wait.asyncmark(0); // will wait for third block, including bar()
   }
