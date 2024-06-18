#include <uapi/linux/perf_event.h>

#define DEFERRED_SPLIT_ISOLATED 1

#define BUFFER_SIZE	32 /* 128: 1MB */
#define CPUS_PER_SOCKET 20
#define MAX_MIGRATION_RATE_IN_MBPS  2048 /* 2048MB per sec */


/* pebs events */
#define DRAM_LLC_LOAD_MISS  0x1d3
#define REMOTE_DRAM_LLC_LOAD_MISS   0x2d3
#define NVM_LLC_LOAD_MISS   0x80d1
#define ALL_STORES	    0x82d0
#define ALL_LOADS	    0x81d0
#define STLB_MISS_STORES    0x12d0
#define STLB_MISS_LOADS	    0x11d0

/* tmm option */
#define HTMM_NO_MIG	    0x0	/* unused */
#define	HTMM_BASELINE	    0x1 /* unused */
#define HTMM_HUGEPAGE_OPT   0x2 /* only used */
#define HTMM_HUGEPAGE_OPT_V2	0x3 /* unused */
#define HTMM_NO_DEMOTION    0x4
#define HTMM_LSTM	    0x5
#define HTMM_LSTM_PDLOCK    0x6
#define HTMM_LSTM_DLOCK     0x7
#define HTMM_ESTIMATION     0x8 /* Estimation technique of HTMM */

/**/
#define DRAM_ACCESS_LATENCY 80
#define NVM_ACCESS_LATENCY  270
#define CXL_ACCESS_LATENCY  170
#define DELTA_CYCLES	(NVM_ACCESS_LATENCY - DRAM_ACCESS_LATENCY)

#define pcount 30
/* only prime numbers */
static const unsigned int pebs_period_list[pcount] = {
    199,    // 200 - min
    293,    // 300
    401,    // 400
    499,    // 500
    599,    // 600
    701,    // 700
    797,    // 800
    907,    // 900
    997,    // 1000
    1201,   // 1200
    1399,   // 1400
    1601,   // 1600
    1801,   // 1800
    1999,   // 2000
    2503,   // 2500
    3001,   // 3000
    3499,   // 3500
    4001,   // 4000
    4507,   // 4507
    4999,   // 5000
    6007,   // 6000
    7001,   // 7000
    7993,   // 8000
    9001,   // 9000
    10007,  // 10000
    12007,  // 12000
    13999,  // 14000
    16001,  // 16000
    17989,  // 18000
    19997,  // 20000 - max
};

#define pinstcount 5
/* this is for store instructions */
static const unsigned int pebs_inst_period_list[pinstcount] ={
    100003, // 0.1M
    300007, // 0.3M
    600011, // 0.6M
    1000003,// 1.0M
    1500003,// 1.5M
};

struct htmm_event {
    struct perf_event_header header;
    __u64 ip;
    __u32 pid, tid;
    __u64 addr;
};

enum events {
    DRAMREAD = 0,
    NVMREAD = 1,
    MEMWRITE = 2,
    TLB_MISS_LOADS = 3,
    TLB_MISS_STORES = 4,
    CXLREAD = 5, // emulated by remote DRAM node
    N_HTMMEVENTS
};

/* htmm_core.c */
extern void htmm_mm_init(struct mm_struct *mm);
extern void htmm_mm_exit(struct mm_struct *mm);
extern void __prep_transhuge_page_for_htmm(struct mm_struct *mm, struct page *page);
extern void prep_transhuge_page_for_htmm(struct vm_area_struct *vma,
					 struct page *page, unsigned long virtual_address);
extern void clear_transhuge_pginfo(struct page *page);
extern void copy_transhuge_pginfo(struct page *page,
				  struct page *newpage);
extern pginfo_t *get_compound_pginfo(struct page *page, unsigned long address);

extern void check_transhuge_cooling(void *arg, struct page *page, bool locked);
extern void check_base_cooling(pginfo_t *pginfo, struct page *page, bool locked);
extern int set_page_coolstatus(struct page *page, pte_t *pte, struct mm_struct *mm, unsigned long fault_address);

extern void set_lru_adjusting(struct mem_cgroup *memcg, bool inc_thres);

extern void update_pginfo(pid_t pid, unsigned long address, enum events e);
extern void bpf_register_memory_access(unsigned long address, unsigned long memtier, unsigned long bottom_accesses, unsigned long ltm_accesses, unsigned long bucket_idx);
extern void bpf_register_memory_access_ltm(unsigned long address, unsigned long bottom_accesses, unsigned long memtier, unsigned long hot_threshold, unsigned long total_accesses);
//extern void bpf_register_memory_access_ltm(unsigned long address, unsigned long stm_accesses, unsigned long ltm_accesses, unsigned long total_accesses);
extern void bpf_register_cooling(unsigned long address, unsigned long total_accesses);
extern void bpf_register_adaptation(unsigned long warm_idx, unsigned long hot_idx, unsigned long total_accesses);
extern void bpf_register_page_alloc(unsigned long virtual_address, unsigned long hugepage, unsigned long total_accesses);
extern void bpf_register_page_add_to_mig_queue_promotion(unsigned long virtual_address,
		unsigned long total_accesses, 
		unsigned long recent_accesses, unsigned long bottom_accesses);
extern void bpf_register_page_add_to_mig_queue_demotion(unsigned long virtual_address,
		unsigned long total_accesses,
		unsigned long recent_accesses, unsigned long bottom_accesses);

extern bool deferred_split_huge_page_for_htmm(struct page *page);
extern unsigned long deferred_split_scan_for_htmm(struct mem_cgroup_per_node *pn,
						  struct list_head *split_list);
extern void putback_split_pages(struct list_head *split_list, struct lruvec *lruvec);

extern bool check_split_huge_page(struct mem_cgroup *memcg, struct page *meta, bool hot);
extern bool move_page_to_deferred_split_queue(struct mem_cgroup *memcg, struct page *page);

extern void move_page_to_active_lru(struct page *page, unsigned long total_accesses, unsigned long stm, unsigned long ltm);
extern void move_page_to_inactive_lru(struct page *page, unsigned long total_accesses, unsigned long stm, unsigned long ltm);


extern struct page *get_meta_page(struct page *page);
extern unsigned int get_accesses_from_idx(unsigned int idx);
extern int get_skew_idx(unsigned long num);
extern void uncharge_htmm_pte(pte_t *pte, struct mem_cgroup *memcg);
extern void uncharge_htmm_page(struct page *page, struct mem_cgroup *memcg);
extern void charge_htmm_page(struct page *page, struct mem_cgroup *memcg);


extern void set_lru_split_pid(pid_t pid);
extern void adjust_active_threshold(pid_t pid);
extern void set_lru_cooling_pid(pid_t pid);

/* htmm_sampler.c */
extern int ksamplingd_init(pid_t pid, int node);
extern void ksamplingd_exit(void);

extern int process_pid;
extern void *memcg;

static unsigned int get_idx(unsigned long num)
{
    unsigned int cnt = 0;
   
    num++;
    while (1) {
	num = num >> 1;
	if (num)
	    cnt++;
	else	
	    return cnt;
	
	if (cnt == 15)
	    break;
    }

    return cnt;
}

static inline bool decide_ltm_stm(unsigned long stm_accesses, unsigned long ltm_accesses)
{
	if ((stm_accesses * 3) < (ltm_accesses) &&
			(stm_accesses * 15) > ltm_accesses) {
		return true; 
	}
	return false;
}


static inline unsigned long update_ltm(unsigned long current_ltm)
{
	return ((current_ltm * 9) / 10);
}


static inline bool inspect_page_migration_lock(pginfo_t *pginfo, int htmm_mode)
{
	return true;
}

static inline bool inspect_hugepage_migration_lock(struct page *meta_page, int htmm_mode)
{
	return true;
}

static inline unsigned int compute_idx_lstm(unsigned long stm, unsigned long ltm)
{
	unsigned long accesses = stm;
	unsigned int bucket_idx;
	if (decide_ltm_stm(stm, ltm)) {
		accesses = ltm / 9;
	}
	bucket_idx = get_idx(accesses);
	return bucket_idx; 
}

static inline unsigned int compute_idx_estimation(unsigned long nr_samples, unsigned long last_cooling_sample, unsigned long recent_accesses, unsigned long bottom_accesses, unsigned long htmm_cooling_period)
{
	long long alpha, estimation, estimation_1;
	unsigned int bucket_idx;
	alpha = nr_samples - last_cooling_sample; 
	estimation_1 = (long long)recent_accesses - (long long)(alpha * (long long)(bottom_accesses) / htmm_cooling_period);
	if (estimation_1 < 0)
		estimation_1 = 0;
	else
		estimation_1 = estimation_1 / 2;

	estimation = bottom_accesses + estimation_1;
	bucket_idx = get_idx(estimation);
	return bucket_idx;

	//bucket_idx = get_idx(bottom_accesses);
	//return bucket_idx;
}

static inline unsigned int compute_idx_memtis(unsigned long stm)
{
	unsigned int bucket_idx;

	bucket_idx = get_idx(stm);
	return bucket_idx;
}

static inline unsigned int compute_idx(unsigned long nr_samples, unsigned long last_cooling_sample,
		unsigned long recent_accesses, unsigned long bottom_accesses, unsigned long ltm,
		unsigned long htmm_cooling_period, int htmm_mode)
{
	if (htmm_mode == HTMM_ESTIMATION)
		return compute_idx_estimation(nr_samples, last_cooling_sample, recent_accesses, bottom_accesses, htmm_cooling_period);
	if (htmm_mode == HTMM_LSTM || htmm_mode == HTMM_LSTM_DLOCK || htmm_mode == HTMM_LSTM_PDLOCK)
		return compute_idx_lstm(recent_accesses, ltm);
	return compute_idx_memtis(recent_accesses); 
}

static inline unsigned long get_sample_period(unsigned long cur) {
    if (cur < 0)
	return 0;
    else if (cur < pcount)
	return pebs_period_list[cur];
    else
	return pebs_period_list[pcount - 1];
}

static inline unsigned long get_sample_inst_period(unsigned long cur) {
    if (cur < 0)
	return 0;
    else if (cur < pinstcount)
	return pebs_inst_period_list[cur];
    else
	return pebs_inst_period_list[pinstcount - 1];
}
#if 1
static inline void increase_sample_period(unsigned long *llc_period,
					  unsigned long *inst_period) {
    unsigned long p;
    p = *llc_period;
    if (++p < pcount)
	*llc_period = p;
    
    p = *inst_period;
    if (++p < pinstcount)
	*inst_period = p;
}

static inline void decrease_sample_period(unsigned long *llc_period,
					  unsigned long *inst_period) {
    unsigned long p;
    p = *llc_period;
    if (p > 0)
	*llc_period = p - 1;
    
    p = *inst_period;
    if (p > 0)
	*inst_period = p - 1;
}
#else
static inline unsigned int increase_sample_period(unsigned int cur,
						  unsigned int next) {
    do {
	cur++;
    } while (pebs_period_list[cur] < next && cur < pcount);
    
    return cur < pcount ? cur : pcount - 1;
}

static inline unsigned int decrease_sample_period(unsigned int cur,
						  unsigned int next) {
    do {
	cur--;
    } while (pebs_period_list[cur] > next && cur > 0);
    
    return cur;
}
#endif


/* htmm_migrater.c */
#define HTMM_MIN_FREE_PAGES 256 * 10 // 10MB
extern unsigned long get_nr_lru_pages_node(struct mem_cgroup *memcg, pg_data_t *pgdat);
extern void add_memcg_to_kmigraterd(struct mem_cgroup *memcg, int nid);
extern void del_memcg_from_kmigraterd(struct mem_cgroup *memcg, int nid);
extern unsigned long get_memcg_demotion_watermark(unsigned long max_nr_pages);
extern unsigned long get_memcg_promotion_watermark(unsigned long max_nr_pages);
extern void kmigraterd_wakeup(int nid);
extern int kmigraterd_init(void);
extern void kmigraterd_stop(void);
extern void bpf_demotion_loop_hook(unsigned long demotion_ctr, unsigned long nr_reclaimed); 
extern void bpf_promotion_loop_hook(unsigned long promotion_ctr, unsigned long nr_promoted); 
extern void bpf_log_estimate_values_access(unsigned long page_pointer, long long estimation, unsigned long htmm_cooling_period, unsigned long last_cooling_sample, unsigned long total_accesses);
extern void __adjust_active_threshold(struct mem_cgroup *memcg);

