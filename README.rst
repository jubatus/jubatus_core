jubatus_core
============

jubatus_core is the core library of Jubatus.

See http://jubat.us/ for details of Jubatus.

How to install
-----------

We officially support Ubuntu Server 12.04 LTS (x86_64) and Red Hat Enterprise Linux 6.2 or later (x86_64).

If you have already installed Jubatus 0.6.0 or later, you can already use jubatus_core.
`QuickStart <http://jubat.us/en/quickstart.html>`_ describes how to install Jubatus.
If you do not want to install whole Jubatus, you can install jubatus_core only.
Before installation, you should install msgpack and oniguruma (oniguruma is optional).
Then type as following:

::

    wget -O jubatus_core-master.tar.gz https://github.com/jubatus/jubatus_core/archive/master.tar.gz
    tar xf jubatus_core-master.tar.gz
    cd jubatus_core-master
    ./waf configure --prefix=<prefix>
    ./waf
    ./waf --checkall
    ./waf install


If you do not need oniguruma, type

::

    ./waf configure --regexp-library=none --prefix=<prefix>


instead of

::

    ./waf configure --prefix=<prefix>

If you want to use re2 instead of oniguruma, add ``--regexp-library=re2`` to ``./waf configure``.

License
-------

LGPL 2.1

Third-party library included in jubatus_core
----------------------------------------------

Jubatus source tree includes following third-party library.

- Eigen_ (mainly under MPL2 License, while some codes are under LPGL2.1 or LGPL2.1+)

.. _Eigen: http://eigen.tuxfamily.org

- A fork of pficommon_ (placed under jubatus_core's jubatus/core/util/. New BSD License)

.. _pficommon: https://github.com/pfi/pficommon/

Update history
--------------

Update history can be found from `ChangeLog <https://github.com/jubatus/jubatus_core/blob/master/ChangeLog.rst>`_.

Contributors
------------

Contributors are listed at https://github.com/jubatus/jubatus/contributors and https://github.com/jubatus/jubatus_core/contributors.
