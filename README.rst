CStreamingBencodeReader
=======================
.. image:: https://travis-ci.org/willemt/CStreamingBencodeReader.png
   :target: https://travis-ci.org/willemt/CStreamingBencodeReader

.. image:: https://coveralls.io/repos/willemt/CStreamingBencodeReader/badge.png?branch=master
  :target: https://coveralls.io/r/willemt/CStreamingBencodeReader?branch=master

What?
-----
A bencode reader that loves working with streams.

Written in C with a BSD license.

How does it work?
-----------------

See bencode.h for documentation.

To see the module in action check out:

* Unit tests within test_bencode.c

Building
--------
$make

Tradeoffs
---------
Because of its stream friendly nature, CStreamingBencodeReader needs to make occasional calls to malloc(). Seeing as it tries its best to keep these calls to a minimum, this might be OK for your needs.

Otherwise please check out https://github.com/willemt/CHeaplessBencodeReader which doesn't use the heap at all! (ie. its got it's own tradeoffs)
