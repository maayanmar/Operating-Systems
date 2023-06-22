#include "VirtualMemory.h"
#include "PhysicalMemory.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

/******************************* Constants ***********************************/

#define FAILURE 0
#define SUCCESS 1

/******************************* Useful Macros ***********************************/

/* 
 * Used to calculate the offset in case virtual address width is not 
 * divisible by offset width.
 */
#define VIRTUAL_ADDRESS_REMAINDER VIRTUAL_ADDRESS_WIDTH % OFFSET_WIDTH

/*
 * Used to calculate the physical address based on the provided index and offset
 * values. This macro is designed to simplify the process of calculating physical
 * addresses and provide a more concise and readable representation of the underlying
 * computation. NOTE: this macro also used to shift left only when offset = 0.
 */
#define CALCULATE_PHYSICAL_ADDR(index, offset) ((index << OFFSET_WIDTH) + offset)

/*
 * Used to extract the offset from a given address. This macro is designed to simplify
 * the process of extracting the offset and provide a more concise and readable
 * representation of the underlying computation.
 */
#define EXTRACT_OFFSET(addr) (addr & ((1 << OFFSET_WIDTH) - 1))

/*
 * Used to extract the page from a given address. This macro is designed to simplify
 * the process of extracting the page and provide a more concise and readable
 * representation of the underlying computation.
 */
#define EXTRACT_PAGE(addr) (addr >> OFFSET_WIDTH)

/*
 * Used to determine whether a given node is empty. This macro is designed to simplify
 * and provide a more concise and readable representation of the underlying computation.
 * !is_leaf && is_empty determines wheter the node is empty
 * node.index != 0 && node.index != context->parent determines whether the node is
 * the root or the parent of the current node (in which case it is not empty).
 */
#define IS_EMPTY_TABLE (!is_leaf && is_empty && node.index != 0 \
  && node.index != context->parent) 

/**
 * Translate virtual address to physical address
 * @param addr address to translate
 * @return 1 on success, 0 on failure
 */
int translate(uint64_t *addr);

/**
 * Fills the given frame with zeros
 * @param index index of frame to clear
 */
void clear_frame(uint64_t index);

/******************************* API impl ***********************************/

void VMinitialize() { clear_frame(0); }

int VMread(uint64_t addr, word_t *value) {
  if (addr >= VIRTUAL_MEMORY_SIZE || translate(&addr) == FAILURE ||
      value == NULL) {
    return FAILURE;
  }
  // read the value from the physical memory
  PMread(addr, value);
  return SUCCESS;
}

int VMwrite(uint64_t addr, word_t value) {
  if (addr >= VIRTUAL_MEMORY_SIZE || translate(&addr) == FAILURE) {
    return FAILURE;
  }
  // write the value to the physical memory
  PMwrite(addr, value);
  return SUCCESS;
}

/*******************************/

void clear_frame(uint64_t index) {
  uint64_t addr = CALCULATE_PHYSICAL_ADDR(index, 0);
  for (int i = 0; i < PAGE_SIZE; i++) {
    PMwrite(addr + i, 0);
  }
}

/**
 * Finds the frame containing the page. If page is not in memory, evicts a frame
 * and restores the page into it.
 * @param page the page to insert
 * @param node the current node
 */
uint64_t find_frame(uint64_t page, uint64_t node, int level);


int translate(uint64_t *addr) {
  uint64_t offset = EXTRACT_OFFSET(*addr);
  uint64_t page = EXTRACT_PAGE(*addr);
  *addr = CALCULATE_PHYSICAL_ADDR(find_frame(page, 0, 0), offset);
  // if we got frame 0 it's considered a failure, unless the depth is 0
  return (*addr == offset && TABLES_DEPTH > 0) ? FAILURE : SUCCESS;
}


/******************************* Frame methods *******************************/

/**
 * Calculates the offset for a given page and level in the table tree traversal.
 * there is further explanation about the function in the implementation.
 */
uint64_t calculate_offset(uint64_t page, uint64_t level);

/**
 * Get the child of a node
 * @param node the node
 * @param index the index of the child
 * @return the child
 */
uint64_t get_child(uint64_t node, int index);

/**
 * Finds a frame, evicts it if necessary and inserts the new page into it.
 * @param parent_node the parent node of the page (used to avoid evicting it)
 * @param index_in_parent the index of the page in the parent node
 * @param page the page to insert
 * @param is_leaf whether the page is a leaf
 * @return the index of the frame containing the page (after insertion)
 */
uint64_t page_fault(uint64_t parent_node, int index_in_parent, uint64_t page,
                                                                bool is_leaf);

uint64_t find_frame(uint64_t page, uint64_t node, int level) {
  // if we reached the page, return the node
  if (level == TABLES_DEPTH) { return node; }

  // calculate the offset of the page in the current node
  uint64_t offset = calculate_offset(page, level);

  // get the child of the node
  uint64_t child = get_child(node, offset);
  // if the child is empty, create a new frame and insert the page into it
  if (child == 0) { 
    child = page_fault(node, offset, page, level + 1 >= TABLES_DEPTH);
  }
  // continue the search in the child
  return find_frame(page, child, level + 1);
}


//######## Structs for Depth-First Search (DFS) execution ########// 

/**
 * Stores a frame and the address of its pointer in physical memory
 * @field index the index of the frame
 * @field addr the address of the pointer to the frame
*/
typedef struct {
  uint64_t index;
  uint64_t addr;
} frame_ptr_t;


/**
 * Structure representing the context for Depth-First Search (DFS) execution.
 * It contains various parameters and information used during the traversal and
 * page insertion process.
 */
typedef struct {
 
  uint64_t page;                // Index of the page to be restored.

  uint64_t parent;              // Index of the parent node.

  frame_ptr_t unused_frame;     /* Represents an unused table map frame.
                                 * (excluding the parent frame).
                                 * This frame is available for page insertion.
                                 */

  frame_ptr_t max_index_frame;  /* Represents the frame with the maximum index.
                                 * if a new frame needs to be created, it will
                                 * have an index greater than this maximum index
                                 * frame.
                                 */

  frame_ptr_t max_dist_frame;   /* Represents the frame with the maximum cyclic
                                 * distance from the 'page' being processed.
                                 * This frame might be selected for eviction
                                 * during the page insertion process.
                                 */

  uint64_t max_dist_page;        // The page stored in the 'max_dist_frame'.

  uint64_t max_dist;             // The maximum cyclic distance from the 'page'.

} dfs_context_t;

//################################################################// 

/**
 * Selects a frame to evict and insert the new page into.
 * @param context context of the dfs
 * @param node the current node
 * @param level the current level of search
 * @param virtual_addr the virtual address of the current node
 * @return 1 if found an unused table map, 0 otherwise (use max_index_frame or
 * max_dist_frame)
 */
int dfs(dfs_context_t *context, frame_ptr_t node, int level, uint64_t virtual_addr);

/**
 * Set the child of a node
 * @param node the node
 * @param index the index of the child
 * @param child the child
 */
void set_child(uint64_t node, int index, uint64_t child);


uint64_t page_fault(uint64_t parent_node, int index_in_parent, uint64_t page,
                                                                bool is_leaf) {
  // initialize the dfs context, frame & remove_from_parent flag
  dfs_context_t context = {page, parent_node, 0, 0, 0, 0, 0};
  frame_ptr_t frame = {0, 0};
  bool remove_from_parent = true;
  
  /* search for (ordered by priority):
   * 1. unused frame  
   * 2. frame with maximum index
   * 3. frame with maximum distance from the page
   */

  if (dfs(&context, frame, 0, 0) == SUCCESS) {
    frame = context.unused_frame;
    
  } else if (context.max_index_frame.index + 1 < NUM_FRAMES) {
    frame = context.max_index_frame;
    frame.index++;
    remove_from_parent = false;
    
  } else {
    frame = context.max_dist_frame;
    // evict the page in the frame
    PMevict(frame.index, context.max_dist_page);
  }

  // remove from old parent
  if (remove_from_parent) { PMwrite(frame.addr, 0); }

  // if next level is a page, restore it otherwise clear the frame
  if (is_leaf) { 
    PMrestore(frame.index, page);
  } else {
    clear_frame(frame.index);
  }

  // write to new parent
  set_child(parent_node, index_in_parent, frame.index);

  // return the address of the new frame
  return frame.index;
}

/********************* Table tree traversal and translation *******************/

/**
 * Get pointer to the child of a node 
 * @param node the node
 * @param index the index of the child
 * @return the pointer to the child
 */
frame_ptr_t get_child_ptr(frame_ptr_t node, int index);

/**
 * Executed for each node in the dfs. 
 * Finds:
 *    1. an unused table map frame if exists
 *    2. the frame with the max index
 *    3. the frame containing a page with the max cyclic distance from context.page
 * 
 * @param context the dfs context
 * @param node the current node
 * @param level the current level in the dfs
 * @param is_empty true if node has no children (when node is not a leaf)
 * @param page the page in node (when node is a leaf)
 * @return 1 if found an unused table map, 0 otherwise
 */
int update_context(dfs_context_t *context, frame_ptr_t node, int level,
                                          bool is_empty, uint64_t page);


int dfs(dfs_context_t *context, frame_ptr_t node, int level, uint64_t virtual_addr) {

  bool is_empty = true;
  // if level is not a page inspect the node, otherwise upate the context and return
  if (level < TABLES_DEPTH) {

    // update the virtual address
    virtual_addr = CALCULATE_PHYSICAL_ADDR(virtual_addr, 0);

    // visit each child of the node
    for (int i = 0; i < PAGE_SIZE; i++) {
      frame_ptr_t child = get_child_ptr(node, i);

      // for each child, continue the search in it
      if (child.index != 0) { 
        is_empty = false;

        if (dfs(context, child, level + 1, virtual_addr + i) != FAILURE) {
          return SUCCESS;
        }
      }
    }
  }
  return update_context(context, node, level, is_empty, virtual_addr);
}

/**
 * Calculate the cyclic distance between two pages
 */
int calculate_cyclic_distance(uint64_t page, uint64_t other_page);

int update_context(dfs_context_t *context, frame_ptr_t node, int level,
                                          bool is_empty, uint64_t page) {

  // check if node is a leaf  (used also in IS_EMPTY_TABLE macro)
  bool is_leaf = (level >= TABLES_DEPTH);

  // check if node is an unused table map
  if (IS_EMPTY_TABLE) { 

    context->unused_frame = node;
    return SUCCESS;
  }
  
  // update max index frame if needed
  if (node.index > context->max_index_frame.index) {  
    context->max_index_frame = node;
  }

  // update max distance page (and corresponding frame) if needed
  if (is_leaf) {          
    int dist = calculate_cyclic_distance(context->page, page);

    if ((uint64_t)dist > context->max_dist) {
      context->max_dist = dist;
      context->max_dist_frame = node;
      context->max_dist_page = page;
    }
  }

  return FAILURE;
}

/************************* Virtual Getters & Setters **************************/

uint64_t get_child(uint64_t node, int index) {
  word_t child;
  PMread(CALCULATE_PHYSICAL_ADDR(node, index), &child);
  return child;
}

frame_ptr_t get_child_ptr(frame_ptr_t node, int index) {
  frame_ptr_t child = {0, CALCULATE_PHYSICAL_ADDR(node.index, index)};
  PMread(child.addr, (word_t *)&child.index);
  return child;
}

void set_child(uint64_t node, int index, uint64_t child) {
  PMwrite(CALCULATE_PHYSICAL_ADDR(node, index), child);
}

int calculate_cyclic_distance(uint64_t page, uint64_t other_page) {
  // the formula is: min(|page - other_page|, NUM_PAGES - |page - other_page|)
  int dist = abs((int)(page - other_page));
  return (dist < (NUM_PAGES - dist) ? dist : NUM_PAGES - dist);
}

/**
 * Calculates the offset for a given page and level in the table tree traversal.
 *
 * @param page  The page number.
 * @param level The current level in the table tree traversal.
 * @return      The offset value.
 *
 * The function computes the bit index where the current level offset starts by
 * adding 2 to the current level and multiplying it by the OFFSET_WIDTH. This is
 * done to skip the root and the current level. The function then subtracts the
 * remainder of dividing the virtual address by the OFFSET_WIDTH from the bit index.
 * Next, it extracts the offset from the page by right-shifting the page value by
 * the difference between VIRTUAL_ADDRESS_WIDTH and the bit index. Finally, the
 * extracted offset is obtained by applying a bitwise AND operation between the
 * calculated value EXTRACT_OFFSET to ensure that only the bits within the
 * offset width are preserved. The resulting offset value is returned.
 *
 * Edge Cases:
 * When VIRTUAL_ADDRESS_WIDTH is not divisible by OFFSET_WIDTH, the function may
 * produce different results due to the remainder in the calculation of the bit index.
 * This can lead to variations in the extracted offset, potentially affecting the
 * subsequent operations in the program. It is important to consider this behavior
 * and its implications when analyzing the function's output in such cases.
 */
uint64_t calculate_offset(uint64_t page, uint64_t level) {
  uint64_t bit_index = (level + 2) * OFFSET_WIDTH - VIRTUAL_ADDRESS_REMAINDER;
  uint64_t shifted_page = page >> (VIRTUAL_ADDRESS_WIDTH - bit_index);
  return EXTRACT_OFFSET(shifted_page);
}