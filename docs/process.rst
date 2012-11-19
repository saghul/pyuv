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


    .. py:classmethod:: disable_stdio_inheritance

        Disables inheritance for file descriptors / handles that this process
        inherited from its parent. The effect is that child processes spawned by
        this process don't accidentally inherit these handles.

        It is recommended to call this function as early in your program as possible,
        before the inherited file descriptors can be closed or duplicated.

        Note that this function works on a best-effort basis: there is no guarantee
        that libuv can discover all file descriptors that were inherited. In general
        it does a better job on Windows than it does on unix.

    .. py:method:: spawn(signal)

        :param string file: File to be executed.

        :param callable exit_callback: Callback to be called when the process exits.

        :param tuple args: Arguments to be passed to the executable.

        :param dict env: Overrides the environment for the child process. If none is
            specified the one from the parent is used.

        :param string cwd: Specifies the working directory where the child process will
            be executed.

        :param int uid: UID of the user to be used if flag ``UV_PROCESS_SETUID`` is used.

        :param int gid: GID of the group to be used if flag ``UV_PROCESS_SETGID`` is used.

        :param int flags: Available flags:

            - ``UV_PROCESS_SETUID``: set child UID
            - ``UV_PROCESS_SETGID``: set child GID
            - ``UV_PROCESS_WINDOWS_VERBATIM_ARGUMENTS``: pass arguments verbatim, that is, not enclosed
              in double quotes (Windows)
            - ``UV_PROCESS_DETACHED``: detach child process from parent

        :param list stdio: Sequence containing ``StdIO`` containers which will be used to pass stdio
            handles to the child process. See the ``StdIO`` class documentation for for information.

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

    .. py:attribute:: pid

        *Read only*

        PID of the spawned process.

    .. py:attribute:: active

        *Read only*

        Indicates if this handle is active.

    .. py:attribute:: closed

        *Read only*

        Indicates if this handle is closing or already closed.


.. py:class:: StdIO([[[stream], fd], flags])

    :param object stream: Stream object.

    :param int fd: File descriptor.

    :param int flags: Flags.

    Create a new container for passing stdio to a child process. Stream can be any stream object,
    that is ``TCP``, ``Pipe`` or ``TTY``. An arbitrary file descriptor can be passed by setting
    the ``fd`` parameter.

    The operation mode is selected by setting the ``flags`` parameter:

    - UV_IGNORE: this container should be ignored.
    - UV_CREATE_PIPE: indicates a pipe should be created. UV_READABLE_PIPE and UV_WRITABLE_PIPE determine the direction of flow, from the child process' perspective. Both flags may be specified to create a duplex data stream.
    - UV_INHERIT_FD: inherit the given file descriptor in the child.
    - UV_INHERIT_STREAM: inherit the file descriptor of the given stream in the child.


