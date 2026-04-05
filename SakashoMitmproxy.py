import mitmproxy.http
from mitmproxy import ctx
from transpiled.SakashoObfuscation import SakashoObfuscation
import array

SESSION_ID_COOKIE_NAME = "player_session_id"

class SakashoMitmproxy:
    # ----------------------------
    # Utilities
    # ----------------------------

    def get_session_id(self, flow: mitmproxy.http.HTTPFlow):
        """
        Retrieve the session ID from cookies.
        It first checks the response cookies, then the request cookies.
        If not found, returns None.

        Args:
            flow (mitmproxy.http.HTTPFlow): The HTTP flow.

        Returns:
            str or None: The session ID if found, else None.
        """
        # Check response cookies first
        if flow.response:
            cookies = flow.response.cookies.get(SESSION_ID_COOKIE_NAME)
            if cookies:
                return cookies[0] if isinstance(cookies, tuple) else cookies

        # Then request cookies
        if flow.request:
            cookies = flow.request.cookies.get(SESSION_ID_COOKIE_NAME)
            if cookies:
                return cookies[0] if isinstance(cookies, tuple) else cookies

        return None

    def should_process_flow(self, flow: mitmproxy.http.HTTPFlow) -> bool:
        """
        Determine whether to process the flow based on specific criteria.
        This function checks if the User-Agent contains "SakashoClient".

        Args:
            flow (mitmproxy.http.HTTPFlow): The HTTP flow.

        Returns:
            bool: True if the flow should be processed, otherwise False.
        """
        # Check if User-Agent contains "SakashoClient"
        user_agent = flow.request.headers.get("User-Agent", "")
        if "SakashoClient" not in user_agent:
            return False

        # Skip /v1/session specifically
        if flow.request.path == "/v1/session":
            return False

        return True

    def build_obfuscator(self, session_id: str) -> SakashoObfuscation:
        obfs = SakashoObfuscation()
        obfs.initialize_miitomo(session_id)
        return obfs

    # ----------------------------
    # mitmproxy hooks
    # ----------------------------

    def request(self, flow: mitmproxy.http.HTTPFlow):
        """
        Handle the HTTP request.
        Encode the request body if it starts with "{".
        """
        session_id = self.get_session_id(flow)
        if not session_id or not self.should_process_flow(flow):
            return

        if not flow.request.content:
            return

        # Only encode JSON-like payloads
        if not flow.request.content.startswith(b"{"):
            return

        try:
            obfs = self.build_obfuscator(session_id)

            pos_out = array.array("i", [0])
            encoded = obfs.encode(
                flow.request.content,
                len(flow.request.content),
                pos_out,
            )
            if encoded is None:
                raise RuntimeError("encode() returned None")

            content = encoded[: pos_out[0]]
            flow.request.content = bytes(content)
            flow.metadata["sakasho_obfs"] = obfs

            ctx.log.info(f"Encoded request {flow.id}")

        except Exception as e:
            ctx.log.error(f"Request encode failed ({flow.id}): {e}")

    def response(self, flow: mitmproxy.http.HTTPFlow):
        """
        Handle the HTTP response.
        Decode the response body if applicable.
        """
        if not flow.response or not flow.response.content:
            return

        # Check if the response contains "<!doctype html>"
        if b"<!doctype html>" in flow.response.content.lower():
            # Truncate the content at the first occurrence of "<!doctype html>"
            doctype_index = flow.response.content.lower().find(b"<!doctype html>")
            flow.response.content = flow.response.content[:doctype_index]
            ctx.log.info(f"Truncated HTML in response {flow.id}")

        session_id = self.get_session_id(flow)
        if not session_id or not self.should_process_flow(flow):
            return

        try:
            obfs = self.build_obfuscator(session_id)

            # Decode the response body.
            decoded = obfs.decode(
                flow.response.content,
                len(flow.response.content),
            )
            if decoded is None:
                raise RuntimeError("decode() returned None")

            flow.response.content = bytes(decoded)
            flow.response.headers["Content-Type"] = "application/json; charset=UTF-8"
            flow.metadata["decoded_response"] = True

            ctx.log.info(f"Decoded response {flow.request.url} ({flow.id})")

        except Exception as e:
            ctx.log.warn(
                f"Response decode failed {flow.request.url} ({flow.id}): {e}"
            )

        # Late-decode request body if it wasn’t JSON initially
        if flow.request and flow.request.content:
            try:
                obfs = self.build_obfuscator(session_id)

                decoded_req = obfs.decode(
                    flow.request.content,
                    len(flow.request.content),
                )
                if decoded_req is not None:
                    flow.request.content = bytes(decoded_req)
                    ctx.log.info(f"Late-decoded request {flow.id}")

            except Exception:
                pass


addons = [
    SakashoMitmproxy()
]
