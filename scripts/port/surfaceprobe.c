/* Can SDL3 wrap a GTK widget's X11 window and give Vulkan a surface for it?
 *
 * That is the whole of the editor's viewport question on Linux, reduced to the
 * part that is not obvious. DXVK Native here is built with the SDL3 WSI
 * backend -- libdxvk_d3d9.so dlopens libSDL3.so.0 and calls
 * SDL_Vulkan_CreateSurface -- so an HWND off Windows is an SDL_Window*, and
 * the viewport needs one that lives inside a wx (GTK) widget rather than
 * being a top level window of its own.
 *
 * Answered on 2026-09-20, on WSLg under XWayland, against the tree's own SDL3
 * (3.3.0) and GTK 3.24.52: **yes, every step.** The output is
 *
 *   1. GTK window and child realized             ok
 *   2. child window has a native X11 XID         ok -- XID 0x600007
 *   3. SDL video up                              ok -- SDL driver x11
 *   4. SDL3 wraps the foreign X11 window         ok -- gtk alloc 640x480;
 *                                                   SDL size -1216073835x32675
 *   5. SDL_SetWindowSize corrects it             ok -- SDL size now 640x480
 *   6. SDL names its instance extensions         ok -- VK_KHR_surface
 *                                                   VK_KHR_xlib_surface
 *   7. a Vulkan instance                         ok
 *   8. a Vulkan surface on the wrapped window    ok
 *
 * **The one trap is step 4.** A wrapped window has no size of its own as far
 * as SDL is concerned: it reports uninitialised numbers and *returns success
 * while doing it*, so nothing tells the caller it is wrong. Passing
 * SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER and _HEIGHT_NUMBER at creation does not
 * help -- that was tried and changed nothing. SDL_SetWindowSize afterwards
 * does. Whatever wraps the viewport has to call it once when the window is
 * made and again on every resize, or DXVK is sizing a swapchain from noise.
 *
 * Run it with GDK_BACKEND=x11; on a Wayland session GTK has no XID to give.
 *
 * Build and run:
 *   see surfaceprobe.sh next to this file
 */
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

/* Before SDL_vulkan.h, which otherwise forward declares the handles itself and
 * leaves out everything an instance is created with. */
#include <vulkan/vulkan.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <stdio.h>
#include <string.h>

static int nFailures = 0;

static void Step( int nStep, const char *pszWhat, int bOk, const char *pszDetail )
{
	printf( "%d. %-44s %s%s%s\n", nStep, pszWhat, bOk ? "ok" : "FAILED",
					( pszDetail && *pszDetail ) ? " -- " : "", pszDetail ? pszDetail : "" );
	if ( !bOk )
	{
		++nFailures;
	}
}

int main( int argc, char **argv )
{
	char szDetail[512];

	/* XWayland rather than the Wayland backend: an XID is the point. */
	gtk_init( &argc, &argv );
	GtkWidget *pTop = gtk_window_new( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_default_size( GTK_WINDOW( pTop ), 640, 480 );
	GtkWidget *pChild = gtk_drawing_area_new();
	gtk_container_add( GTK_CONTAINER( pTop ), pChild );
	gtk_widget_show_all( pTop );
	/* The widget has no GdkWindow until it is realized. */
	gtk_widget_realize( pChild );
	Step( 1, "GTK window and child realized", gtk_widget_get_realized( pChild ), "" );

	GdkWindow *pGdk = gtk_widget_get_window( pChild );
	if ( pGdk == NULL )
	{
		Step( 2, "child has a GdkWindow", 0, "gtk_widget_get_window returned null" );
		return 1;
	}
	/* GTK3 draws most widgets into their parent's window; this is what forces
	 * one of its own, which is what has an XID. GTK4 removed this entirely,
	 * which is why the editor is on GTK3. */
	gdk_window_ensure_native( pGdk );
	if ( !GDK_IS_X11_WINDOW( pGdk ) )
	{
		Step( 2, "child window is an X11 window", 0,
					"not the X11 backend -- run with GDK_BACKEND=x11" );
		return 1;
	}
	const Window xid = gdk_x11_window_get_xid( pGdk );
	snprintf( szDetail, sizeof( szDetail ), "XID 0x%lx", (unsigned long)xid );
	Step( 2, "child window has a native X11 XID", xid != 0, szDetail );

	if ( !SDL_InitSubSystem( SDL_INIT_VIDEO ) )
	{
		Step( 3, "SDL video", 0, SDL_GetError() );
		return 1;
	}
	snprintf( szDetail, sizeof( szDetail ), "SDL driver %s", SDL_GetCurrentVideoDriver() );
	Step( 3, "SDL video up", 1, szDetail );

	/* Wrapping alone leaves SDL with no idea how big the window is -- without
	 * these two it reports uninitialised numbers, and a swapchain would be
	 * built to them. They have to be kept up to date as the widget resizes. */
	GtkAllocation allocation;
	gtk_widget_get_allocation( pChild, &allocation );

	SDL_PropertiesID props = SDL_CreateProperties();
	SDL_SetNumberProperty( props, SDL_PROP_WINDOW_CREATE_X11_WINDOW_NUMBER, (Sint64)xid );
	SDL_SetBooleanProperty( props, SDL_PROP_WINDOW_CREATE_VULKAN_BOOLEAN, true );
	SDL_SetNumberProperty( props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, allocation.width );
	SDL_SetNumberProperty( props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, allocation.height );
	SDL_Window *pWindow = SDL_CreateWindowWithProperties( props );
	SDL_DestroyProperties( props );
	if ( pWindow == NULL )
	{
		Step( 4, "SDL3 wraps the foreign X11 window", 0, SDL_GetError() );
		return 1;
	}
	int nW = -1, nH = -1, nPixW = -1, nPixH = -1;
	const bool bSize = SDL_GetWindowSize( pWindow, &nW, &nH );
	const bool bPix = SDL_GetWindowSizeInPixels( pWindow, &nPixW, &nPixH );
	snprintf( szDetail, sizeof( szDetail ),
						"SDL_Window %p; gtk alloc %dx%d; SDL size %dx%d (%s); in pixels %dx%d (%s)",
						(void*)pWindow, allocation.width, allocation.height,
						nW, nH, bSize ? "ok" : SDL_GetError(),
						nPixW, nPixH, bPix ? "ok" : SDL_GetError() );
	Step( 4, "SDL3 wraps the foreign X11 window", 1, szDetail );

	/* Whether telling SDL the size after the fact puts it right. */
	SDL_SetWindowSize( pWindow, allocation.width, allocation.height );
	SDL_SyncWindow( pWindow );
	int nSetW = -1, nSetH = -1;
	SDL_GetWindowSize( pWindow, &nSetW, &nSetH );
	snprintf( szDetail, sizeof( szDetail ), "SDL size now %dx%d, wanted %dx%d",
						nSetW, nSetH, allocation.width, allocation.height );
	Step( 5, "SDL_SetWindowSize corrects it",
				( nSetW == allocation.width ) && ( nSetH == allocation.height ), szDetail );

	/* The instance extensions SDL says this window needs. */
	Uint32 nExtensions = 0;
	char const * const *ppExtensions = SDL_Vulkan_GetInstanceExtensions( &nExtensions );
	if ( ppExtensions == NULL )
	{
		Step( 6, "SDL names its instance extensions", 0, SDL_GetError() );
		return 1;
	}
	szDetail[0] = '\0';
	for ( Uint32 i = 0; i < nExtensions; ++i )
	{
		strncat( szDetail, ppExtensions[i], sizeof( szDetail ) - strlen( szDetail ) - 2 );
		strncat( szDetail, " ", sizeof( szDetail ) - strlen( szDetail ) - 2 );
	}
	Step( 6, "SDL names its instance extensions", 1, szDetail );

	VkApplicationInfo appInfo;
	memset( &appInfo, 0, sizeof( appInfo ) );
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.apiVersion = VK_API_VERSION_1_0;
	VkInstanceCreateInfo createInfo;
	memset( &createInfo, 0, sizeof( createInfo ) );
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = nExtensions;
	createInfo.ppEnabledExtensionNames = ppExtensions;
	VkInstance instance = VK_NULL_HANDLE;
	const VkResult eResult = vkCreateInstance( &createInfo, NULL, &instance );
	if ( eResult != VK_SUCCESS )
	{
		snprintf( szDetail, sizeof( szDetail ), "vkCreateInstance = %d", (int)eResult );
		Step( 7, "a Vulkan instance", 0, szDetail );
		return 1;
	}
	Step( 7, "a Vulkan instance", 1, "" );

	/* The question the whole probe exists for. */
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	const bool bSurface = SDL_Vulkan_CreateSurface( pWindow, instance, NULL, &surface );
	Step( 8, "a Vulkan surface on the wrapped window", bSurface,
				bSurface ? "" : SDL_GetError() );

	if ( bSurface )
	{
		SDL_Vulkan_DestroySurface( instance, surface, NULL );
	}
	vkDestroyInstance( instance, NULL );
	SDL_DestroyWindow( pWindow );
	SDL_QuitSubSystem( SDL_INIT_VIDEO );

	printf( "\n%s\n", ( nFailures == 0 ) ? "ALL OK: the chain works" : "SOMETHING FAILED" );
	return ( nFailures == 0 ) ? 0 : 1;
}
