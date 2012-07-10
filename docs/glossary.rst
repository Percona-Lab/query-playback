==========
 Glossary
==========

.. glossary::

   InnoDB
      Storage engine which provides ACID-compliant transactions and foreign key support, among others improvements over :term:`MyISAM`. It is the default engine for |MySQL| as of the 5.5 series.

   MyISAM
     Previous default storage engine for |MySQL| for versions prior to 5.5. It doesn't fully support transactions but in some scenarios may be faster than :term:`InnoDB`. Each table is stored on disk in 3 files: `.frm`, `.MYD`, `.MYI`

   tcpdump
     `Tcpdump <http://www.tcpdump.org/>`_ prints out a description of the contents of packets on a network interface that match the boolean expression.

   XtraDB
     *Percona XtraDB* is an enhanced version of the InnoDB storage engine, designed to better scale on modern hardware, and including a variety of other features useful in high performance environments. It is fully backwards compatible, and so can be used as a drop-in replacement for standard InnoDB. More information `here <http://www.percona.com/docs/wiki/Percona-XtraDB:start>`_ .

