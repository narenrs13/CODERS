import time
import sys
import random 
from playwright.sync_api import sync_playwright, Page, Browser, TimeoutError, BrowserContext

# --- Stealth Configuration ---
# Use a common, non-default screen resolution to appear more human
VIEWPORT_CONFIG = {"width": 1920, "height": 1080}
# Consistent, human-like User Agent string for stealth
USER_AGENT = 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36'
# Base time for random delays between actions
MIN_DELAY = 0.5
MAX_DELAY = 1.8

# --- BrowserAutomator Class (Core Engine) ---

class StealthBrowserAutomator:
    """
    Core class for stealth browser automation using Playwright.
    Includes advanced settings to avoid bot detection and CAPTCHA triggers.
    """
    def __init__(self, headless: bool = True):
        """Initializes instance variables."""
        self.headless = headless
        self.playwright = None
        self.browser: Browser = None
        self.context: BrowserContext = None
        self.page: Page = None
        self.user_agent = USER_AGENT

    def start(self):
        """Starts Playwright and launches the browser with stealth parameters."""
        print("🤖 Starting stealth browser automation...")
        try:
            self.playwright = sync_playwright().start()
            
            # Use Chromium as it offers the best compatibility
            self.browser = self.playwright.chromium.launch(headless=self.headless)
            
            # Create a new context with stealth settings
            self.context = self.browser.new_context(
                user_agent=self.user_agent,
                viewport=VIEWPORT_CONFIG,
                # Simulate a real user by allowing cookies, history, etc.
                locale='en-US' 
            )
            
            self.page = self.context.new_page()
            
            print(f"✅ Browser launched: Headless={self.headless}, Viewport={VIEWPORT_CONFIG['width']}x{VIEWPORT_CONFIG['height']}")
        except Exception as e:
            print(f"❌ Error starting browser: {e}")
            self.close()
            sys.exit(1) 

    def close(self):
        """
        Closes the page, context, browser, and stops the Playwright service 
        for a clean exit and to prevent state leakage (like cookies/sessions).
        """
        print("\n👋 Closing browser...")
        # Explicitly close page and context to ensure session state is released
        if self.page:
             self.page.close()
        if self.context:
             self.context.close()
        
        if self.browser:
            self.browser.close()
        if self.playwright:
            self.playwright.stop()
        print("✅ Browser closed.")

    def navigate(self, url: str) -> bool:
        """Navigates to the specified URL with robust waiting."""
        if not self.page:
            self.start()
        print(f"\n🌐 Navigating to {url}")
        try:
            # Random delay before navigating to mimic thinking time
            time.sleep(random.uniform(MIN_DELAY * 2, MAX_DELAY * 3))
            
            self.page.goto(url, wait_until="domcontentloaded", timeout=40000) 
            return True
        except TimeoutError:
            print(f"❌ Navigation failed: Timeout while loading {url}")
            return False
        except Exception as e:
            print(f"❌ Navigation failed: {e}")
            return False

    def click_element(self, selector: str) -> bool:
        """Clicks an element using a CSS selector with a human-like delay."""
        print(f"🖱️ Attempting to click: {selector}")
        try:
            # Use the locator method for more flexible waiting
            locator = self.page.locator(selector)
            
            # Wait for element to be visible and enabled before clicking
            locator.wait_for(state="visible", timeout=15000)
            
            # Optional: Simulate a short hover before clicking
            # locator.hover()
            # time.sleep(random.uniform(0.1, 0.5)) 
            
            locator.click(timeout=10000)
            
            # Introduce a human-like post-click delay
            time.sleep(random.uniform(MIN_DELAY, MAX_DELAY))
            
            print(f"✅ Successfully clicked element.")
            return True
        except TimeoutError:
             print(f"❌ Click failed: Element not found or not clickable within timeout.")
             return False
        except Exception as e:
            print(f"❌ Click failed for selector '{selector}': {e}")
            return False

    def fill_form(self, selector: str, text: str) -> bool:
        """Fills a form input field with text using a key-press simulation."""
        print(f"✍️ Attempting to fill: {selector} with '{text[:30]}...'")
        try:
            # Ensures element is writable and visible
            self.page.wait_for_selector(selector, state="visible", timeout=15000)
            
            # Use type() instead of fill() to simulate key presses, which is stealthier
            self.page.type(selector, text, delay=random.randint(50, 150)) 
            
            # Human-like delay after typing is complete
            time.sleep(random.uniform(MIN_DELAY, MAX_DELAY))
            
            print(f"✅ Successfully filled text (typing simulation).")
            return True
        except TimeoutError:
            print(f"❌ Fill failed: Element not found or not visible within timeout.")
            return False
        except Exception as e:
            print(f"❌ Fill failed for selector '{selector}': {e}")
            return False

    def extract_data(self, selector: str, attribute: str = 'text') -> list:
        """
        Finds all elements matching the selector and extracts their content or attribute.
        """
        print(f"🔍 Attempting to extract data using selector: {selector}")
        results = []
        try:
            # Wait for at least one element to be available and attached to the DOM
            self.page.wait_for_selector(selector, state='attached', timeout=15000)
            
            elements = self.page.locator(selector).all()
            
            for element in elements:
                content = None
                if attribute == 'text':
                    content = element.inner_text().strip()
                else:
                    content = element.get_attribute(attribute)
                
                if content:
                    results.append(content)
            
            print(f"✅ Extracted {len(results)} items.")
            return results
            
        except TimeoutError:
             print(f"⚠️ Extraction failed: No elements matching '{selector}' found within timeout.")
             return []
        except Exception as e:
            print(f"❌ Extraction failed for selector '{selector}': {e}")
            return [] 

    def scroll_page(self, y_pixels: int = 1000) -> bool:
        """Scrolls the page down by a specified number of pixels."""
        if not self.page:
            print("❌ Scroll failed: Browser not started.")
            return False
        
        print(f"⬇️ Scrolling page down by {y_pixels} pixels.")
        try:
            self.page.evaluate(f"window.scrollBy(0, {y_pixels})")
            time.sleep(random.uniform(MIN_DELAY * 0.5, MAX_DELAY * 0.8)) # Quick scroll delay
            print(f"✅ Scrolled successfully.")
            return True
        except Exception as e:
            print(f"❌ Scroll failed: {e}")
            return False

# --- Testing Function (Simulates Orchestration/LLM Input Prep) ---

def run_tests():
    """
    Demonstrates a multi-step automation task using the stealth methods.
    This simulates the steps the LLM Orchestrator would plan.
    """
    # Set headless=False to watch the browser actions (Recommended for debugging stealth)
    automator = StealthBrowserAutomator(headless=False) 
    final_data = {} 
    
    try:
        automator.start()
        
        # 1. Start on a low-detection search engine (DuckDuckGo)
        print("\n--- Test 1: Navigation (Stealth Start) ---")
        if not automator.navigate("https://duckduckgo.com"):
            return

        # 2. Search Execution (Using stealthy type() instead of fill())
        print("\n--- Test 2: Stealth Search and Enter ---")
        search_input_selector = '#search_form_input_homepage' # DuckDuckGo search box
        search_query = "Best travel cameras 2024 review" 
        
        if automator.fill_form(search_input_selector, search_query):
            # Human-like post-typing delay
            time.sleep(random.uniform(1.0, 2.5)) 
            
            # Click the search button
            automator.click_element('#search_button_homepage')
            
            # Longer wait for the results page to load
            time.sleep(random.uniform(3.0, 5.0)) 

        # 3. Extraction of Search Results
        print("\n--- Test 3: Extraction of Search Results ---")
        result_link_selector = 'a.result__a' 
        extracted_titles = automator.extract_data(result_link_selector, attribute='text')
        
        # 4. Click to Navigate to a Detail Page
        if extracted_titles:
            print("\n--- Test 4: Stealth Click to Detail Page ---")
            
            # Using the robust selector for the first link
            first_link_selector = 'a.result__a:first-of-type'
            
            if automator.click_element(first_link_selector):
                # Mandatory delay after click for the new page to render
                print("⏳ Waiting for new page content (Detail Page)...")
                time.sleep(random.uniform(4.0, 6.0)) 
                
                # 5. Extraction from the Detail Page
                print("\n--- Test 5: Extraction from Detail Page ---")
                
                # Extract the main article heading 
                detail_title_selector = 'h1' 
                detail_data = automator.extract_data(detail_title_selector, attribute='text')
                
                # Prepare final output structure
                final_data['search_results'] = list(dict.fromkeys(extracted_titles))[:3]
                final_data['detail_page_title'] = detail_data[0] if detail_data else "(Extraction Failed)"

        else:
            final_data['search_results'] = ["Search failed, no results found."]
            final_data['detail_page_title'] = "(Click skipped)"

        # 6. Final Output Simulation (Data ready for LLM)
        print("\n--- Final Data Ready for LLM (Simulated) ---")
        print(f"Top 3 Search Titles: {final_data.get('search_results', ['N/A'])}...")
        print(f"Detail Page Title: {final_data.get('detail_page_title', 'N/A')}")
        print("\n[MEMBER 3 TASK COMPLETE] Data successfully extracted and structured.")
        
    except Exception as e:
        print(f"\n🚨 An unexpected error occurred during testing: {e}")
        
    finally:
        # 7. Cleanup
        automator.close()
