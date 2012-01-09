.. _process:


.. currentmodule:: pyuv


=====================================================
:py:class:`Process` --- Child process spawning handle
=====================================================


.. py:class:: Process(loop)

    :type loop: :py:class:`Loop`
    :param loop: loop object where this handle runs (accessible through :py:attr:`Check.loop`).

    ``Process`` handles allow spawning child processes which can be controlled (their stdin and
    stdout) with ``Pipe`` handles within an event loop.


    .. py:method:: spawn(signal)

        :param string file: File to be executed.

        :param callable exit_callback: Callback to be called when the process exits.

        :param tuple args: Arguments to be passed to the executable.

        :param dict env: Overrides the environment for the child process. If none is
            specified the one from the parent is used.

        :param string cwd: Specifies the working directory where the child process will
            be executed.

        :param Pipe stdin: Uninitialized ``Pipe`` which will be used as the process stdin.

        :param Pipe stdout: Uninitialized ``Pipe`` which will be used as the process stdout.

        :param Pipe stderr: Uninitialized ``Pipe`` which will be used as the process stderr.

        Spawn the specified child process.

        Exit callback signature: ``callback(process_handle, exit_status, term_signal)``.

    .. py:method:: kill(signal)

        :param int signal: Signal to be sent to the process.

        Send the specified signal to the child process.

    .. py:method:: close([callback])

        :param callable callback: Function that will be called after the ``Process``
            handle is closed.

        Close the ``Process`` handle. After a handle has been closed no other
        operations can be performed on it.

        Callback signature: ``callback(process_handle)``.

    .. py:attribute:: loop

        *Read only*

        :py:class:`Loop` object where this handle runs.

    .. py:attribute:: data

        Any Python object attached to this handle.

    .. py:attribute:: pid

        *Read only*

        PID of the spawned process.


