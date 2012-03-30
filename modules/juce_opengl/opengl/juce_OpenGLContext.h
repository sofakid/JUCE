/*
  ==============================================================================

   This file is part of the JUCE library - "Jules' Utility Class Extensions"
   Copyright 2004-11 by Raw Material Software Ltd.

  ------------------------------------------------------------------------------

   JUCE can be redistributed and/or modified under the terms of the GNU General
   Public License (Version 2), as published by the Free Software Foundation.
   A copy of the license is included in the JUCE distribution, or can be found
   online at www.gnu.org/licenses.

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.rawmaterialsoftware.com/juce for more information.

  ==============================================================================
*/

#ifndef __JUCE_OPENGLCONTEXT_JUCEHEADER__
#define __JUCE_OPENGLCONTEXT_JUCEHEADER__

#include "juce_OpenGLPixelFormat.h"
#include "../native/juce_OpenGLExtensions.h"

//==============================================================================
/**
    A base class that should be implemented by classes which want to render openGL
    on a background thread.

    @see OpenGLContext
*/
class JUCE_API  OpenGLRenderer
{
public:
    OpenGLRenderer() {}
    virtual ~OpenGLRenderer() {}

    /** Called when a new GL context has been created.
        You can use this as an opportunity to create your textures, shaders, etc.
        When the method is invoked, the new GL context will be active.
        Note that this callback will be made on a background thread, so make sure
        that your implementation is thread-safe.
    */
    virtual void newOpenGLContextCreated() = 0;

    /** Called when you should render the next openGL frame.
        Note that this callback will be made on a background thread, so make sure
        that your implementation is thread-safe.
    */
    virtual void renderOpenGL() = 0;

    /** Called when the current openGL context is about to close.
        You can use this opportunity to release any GL resources that you may have
        created.

        Note that this callback will be made on a background thread, so make sure
        that your implementation is thread-safe.

        (Also note that on Android, this callback won't happen, because there's currently
        no way to implement it..)
    */
    virtual void openGLContextClosing() = 0;
};

//==============================================================================
/**
    Creates an OpenGL context, which can be attached to a component.

    To render some OpenGL in a component, you should create an instance of an OpenGLContext,
    and call attachTo() to make it use your component as its render target.
    To free the context, either call detach(), or delete the OpenGLContext.

    @see OpenGLRenderer
*/
class JUCE_API  OpenGLContext
{
public:
    OpenGLContext();

    /** Destructor. */
    ~OpenGLContext();

    //==============================================================================
    /** Gives the context an OpenGLRenderer to use to do the drawing.
        The object that you give it will not be owned by the context, so it's the caller's
        responsibility to manage its lifetime and make sure that it doesn't get deleted
        while the context may be using it. To stop the context using a renderer, just call
        this method with a null pointer.
        Note: This must be called BEFORE attaching your context to a target component!
    */
    void setRenderer (OpenGLRenderer* rendererToUse) noexcept;

    /** Enables or disables the use of the GL context to perform 2D rendering
        of the component to which it is attached.
        If this is false, then only your OpenGLRenderer will be used to perform
        any rendering. If true, then each time your target's paint() method needs
        to be called, an OpenGLGraphicsContext will be used to render it, (after
        calling your OpenGLRenderer if there is one).

        By default this is set to true. If you're not using any paint() method functionality
        and are doing all your rendering in an OpenGLRenderer, you should disable it
        to improve performance.

        Note: This must be called BEFORE attaching your context to a target component!
    */
    void setComponentPaintingEnabled (bool shouldPaintComponent) noexcept;

    /** Sets the pixel format which you'd like to use for the target GL surface.
        Note: This must be called BEFORE attaching your context to a target component!
    */
    void setPixelFormat (const OpenGLPixelFormat& preferredPixelFormat) noexcept;

    /** Provides a context with which you'd like this context's resources to be shared.
        The object passed-in here must not be deleted while the context may still be
        using it! To turn off sharing, you can call this method with a null pointer.
        Note: This must be called BEFORE attaching your context to a target component!
    */
    void setContextToShareWith (const OpenGLContext* contextToShareWith) noexcept;

    //==============================================================================
    /** Attaches the context to a target component.

        If the component is not fully visible, this call will wait until the component
        is shown before actually creating a native context for it.

        When a native context is created, a thread is started, and will be used to call
        the OpenGLRenderer methods. The context will be floated above the target component,
        and when the target moves, it will track it. If the component is hidden/shown, the
        context may be deleted and re-created.
    */
    void attachTo (Component& component);

    /** Detaches the context from its target component and deletes any native resources.
        If the context has not been attached, this will do nothing. Otherwise, it will block
        until the context and its thread have been cleaned up.
    */
    void detach();

    /** Returns true if the context is attached to a component and is on-screen.
        Note that if you call attachTo() for a non-visible component, this method will
        return false until the component is made visible.
    */
    bool isAttached() const noexcept;

    /** Returns the component to which this context is currently attached, or nullptr. */
    Component* getTargetComponent() const noexcept;

    /** Returns the context that's currently in active use by the calling thread, or
        nullptr if no context is active.
    */
    static OpenGLContext* getCurrentContext();

    /** Asynchronously causes a repaint to be made. */
    void triggerRepaint();

    //==============================================================================
    /** Returns the width of this context */
    inline int getWidth() const noexcept                    { return width; }

    /** Returns the height of this context */
    inline int getHeight() const noexcept                   { return height; }

    /** If this context is backed by a frame buffer, this returns its ID number,
        or 0 if the context does not use a framebuffer.
    */
    unsigned int getFrameBufferID() const noexcept;

    /** Returns true if shaders can be used in this context. */
    bool areShadersAvailable() const;

    /** This structure holds a set of dynamically loaded GL functions for use on this context. */
    OpenGLExtensionFunctions extensions;

    //==============================================================================
    /** This retrieves an object that was previously stored with setAssociatedObject().
        If no object is found with the given name, this will return nullptr.
        This method must only be called from within the GL rendering methods.
        @see setAssociatedObject
    */
    ReferenceCountedObject* getAssociatedObject (const char* name) const;

    /** Attaches a named object to the context, which will be deleted when the context is
        destroyed.

        This allows you to store an object which will be released before the context is
        deleted. The main purpose is for caching GL objects such as shader programs, which
        will become invalid when the context is deleted.

        This method must only be called from within the GL rendering methods.
    */
    void setAssociatedObject (const char* name, ReferenceCountedObject* newObject);

    //==============================================================================
    /** Makes this context the currently active one.
        You should never need to call this in normal use - the context will already be
        active when OpenGLRenderer::renderOpenGL() is invoked.
    */
    bool makeActive() const noexcept;

    /** Returns true if this context is currently active for the calling thread. */
    bool isActive() const noexcept;

    //==============================================================================
    /** Swaps the buffers (if the context can do this).
        There's normally no need to call this directly - the buffers will be swapped
        automatically after your OpenGLRenderer::renderOpenGL() method has been called.
    */
    void swapBuffers();

    /** Sets whether the context checks the vertical sync before swapping.

        The value is the number of frames to allow between buffer-swapping. This is
        fairly system-dependent, but 0 turns off syncing, 1 makes it swap on frame-boundaries,
        and greater numbers indicate that it should swap less often.

        Returns true if it sets the value successfully - some platforms won't support
        this setting.
    */
    bool setSwapInterval (int numFramesPerSwap);

    /** Returns the current swap-sync interval.
        See setSwapInterval() for info about the value returned.
    */
    int getSwapInterval() const;

    //==============================================================================
    /** Returns an OS-dependent handle to some kind of underlting OS-provided GL context.

        The exact type of the value returned will depend on the OS and may change
        if the implementation changes. If you want to use this, digging around in the
        native code is probably the best way to find out what it is.
    */
    void* getRawContext() const noexcept;


    //==============================================================================
    /** Draws the currently selected texture into this context at its original size.

        @param targetClipArea  the target area to draw into (in top-left origin coords)
        @param anchorPosAndTextureSize  the position of this rectangle is the texture's top-left
                                        anchor position in the target space, and the size must be
                                        the total size of the texture.
        @param contextWidth     the width of the context or framebuffer that is being drawn into,
                                used for scaling of the coordinates.
        @param contextHeight    the height of the context or framebuffer that is being drawn into,
                                used for vertical flipping of the y coordinates.
    */
    void copyTexture (const Rectangle<int>& targetClipArea,
                      const Rectangle<int>& anchorPosAndTextureSize,
                      int contextWidth, int contextHeight);

   #ifndef DOXYGEN
    class NativeContext;
   #endif

private:
    class CachedImage;
    class Attachment;
    NativeContext* nativeContext;
    OpenGLRenderer* renderer;
    ScopedPointer<Attachment> attachment;
    OpenGLPixelFormat pixelFormat;
    const OpenGLContext* contextToShareWith;
    int width, height;
    bool renderComponents;

    CachedImage* getCachedImage() const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OpenGLContext);
};



#endif   // __JUCE_OPENGLCONTEXT_JUCEHEADER__
