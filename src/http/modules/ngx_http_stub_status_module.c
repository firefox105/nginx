
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


static char *ngx_http_set_status(ngx_conf_t *cf, ngx_command_t *cmd,
                                 void *conf);
    
static ngx_command_t  ngx_http_status_commands[] = {

    { ngx_string("stub_status"),
      NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_http_set_status,
      0,
      0,
      NULL },

      ngx_null_command
};  


    
ngx_http_module_t  ngx_http_stub_status_module_ctx = {
    NULL,                                  /* pre conf */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */
    
    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */
    
    NULL,                                  /* create location configuration */
    NULL                                   /* merge location configuration */
};


ngx_module_t  ngx_http_stub_status_module = {
    NGX_MODULE,
    &ngx_http_stub_status_module_ctx,      /* module context */
    ngx_http_status_commands,              /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init module */
    NULL                                   /* init process */
};


static ngx_int_t ngx_http_status_handler(ngx_http_request_t *r)
{
    size_t        size;
    ngx_int_t     rc;
    uint32_t      ap, hn, ac, rq, rd, wr;
    ngx_buf_t    *b;
    ngx_chain_t   out;
    
    if (r->method != NGX_HTTP_GET && r->method != NGX_HTTP_HEAD) {
        return NGX_HTTP_NOT_ALLOWED;
    }

    rc = ngx_http_discard_body(r);

    if (rc != NGX_OK && rc != NGX_AGAIN) {
        return rc;
    }

    r->headers_out.content_type = ngx_list_push(&r->headers_out.headers);
    if (r->headers_out.content_type == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    r->headers_out.content_type->key.len = 0;
    r->headers_out.content_type->key.data = NULL;
    r->headers_out.content_type->value.len = sizeof("text/plain") - 1;
    r->headers_out.content_type->value.data = (u_char *) "text/plain";

    if (r->method == NGX_HTTP_HEAD) {
        r->headers_out.status = NGX_HTTP_OK;

        rc = ngx_http_send_header(r);

        if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
            return rc;
        }
    }

    size = sizeof("Active connections:  \n") + NGX_INT32_LEN
           + sizeof("server accepts handled requests\n") - 1
           + 6 + 3 * NGX_INT32_LEN
           + sizeof("Reading:  Writing:  Waiting:  \n") + 3 * NGX_INT32_LEN;

    if (!(b = ngx_create_temp_buf(r->pool, size))) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    out.buf = b;
    out.next = NULL;

    ap = *ngx_stat_accepted;
    hn = *ngx_connection_counter;
    ac = *ngx_stat_active;
    rq = *ngx_stat_requests;
    rd = *ngx_stat_reading;
    wr = *ngx_stat_writing;

    b->last = ngx_sprintf(b->last, "Active connections: %d \n", ac);

    b->last = ngx_cpymem(b->last, "server accepts handled requests\n",
                         sizeof("server accepts handled requests\n") - 1);

    b->last = ngx_sprintf(b->last, " %d %d %d \n", ap, hn, rq);

    b->last = ngx_sprintf(b->last, "Reading: %d Writing: %d Waiting: %d \n",
                           rd, wr, ac - (rd + wr));

    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = b->last - b->pos;

    b->last_buf = 1;

    rc = ngx_http_send_header(r);

    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
        return rc;
    }

    return ngx_http_output_filter(r, &out);;
}


static char *ngx_http_set_status(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t  *clcf;

    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_status_handler;

    return NGX_CONF_OK;
}
